/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/renderer.h>
#include <lm/scene.h>
#include <lm/film.h>
#include <lm/scheduler.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

enum class PTMode {
    Naive,
    NEE,
    MIS,
};

enum class SampleMode {
    Pixel,
    Image,
};

template <typename SampleMode, SampleMode sampleMode>
class Renderer_PT : public Renderer {
private:
    Film* film_;
    int maxLength_;
    std::optional<unsigned int> seed_;
    PTMode ptMode_;
    Component::Ptr<scheduler::Scheduler> sched_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(film_, maxLength_, ptMode_, sched_);
    }

    virtual void foreachUnderlying(const ComponentVisitor& visit) override {
        comp::visit(visit, film_);
        comp::visit(visit, sched_);
    }

public:
    virtual bool construct(const Json& prop) override {
        film_ = json::compRef<Film>(prop, "output");
        maxLength_ = json::value<int>(prop, "max_length");
        seed_ = json::valueOrNone<unsigned int>(prop, "seed");
        ptMode_ = [&]() -> PTMode {
            const auto s = json::value<std::string>(prop, "mode", "mis");
            if (s == "naive") {
                return PTMode::Naive;
            }
            else if (s == "nee") {
                return PTMode::NEE;
            }
            else if (s == "mis") {
                return PTMode::MIS;
            }
            LM_UNREACHABLE_RETURN();
        }();
        const auto schedName = json::value<std::string>(prop, "scheduler");
        if constexpr (sampleMode == SampleMode::Pixel) {
            sched_ = comp::create<scheduler::Scheduler>(
                "scheduler::spp::" + schedName, makeLoc("scheduler"), prop);
        }
        else {
            sched_ = comp::create<scheduler::Scheduler>(
                "scheduler::spi::" + schedName, makeLoc("scheduler"), prop);
        }
        return true;
    }

    virtual void render(const Scene* scene) const override {
        // Clear film
        film_->clear();
        const auto size = film_->size();

        // ----------------------------------------------------------------------------------------

        // Dispatch rendering
        const auto processed = sched_->run([&](long long pixelIndex, long long, int threadid) {
            // Per-thread random number generator
            thread_local Rng rng(seed_ ? *seed_ + threadid : math::rngSeed());

            // ------------------------------------------------------------------------------------

            // Sample window
            const auto window = [&]() constexpr -> Vec4 {
                if constexpr (sampleMode == SampleMode::Pixel) {
                    const int x = int(pixelIndex % size.w);
                    const int y = int(pixelIndex / size.w);
                    const auto dx = 1_f / size.w;
                    const auto dy = 1_f / size.h;
                    return { dx * x, dy * y, dx, dy };
                }
                else {
                    return { 0_f, 0_f, 1_f, 1_f };
                }
            }();

            // ------------------------------------------------------------------------------------

            // Path throughput
            Vec3 throughput(1_f);

            // Incident direction and current surface point
            Vec3 wi = {};
            auto sp = SceneInteraction::makeCameraTerminator(window, film_->aspectRatio());

            // Raster position
            Vec2 rasterPos{};

            // Perform random walk
            for (int length = 0; length < maxLength_; length++) {
                // Sample a ray
                const auto s = scene->sampleRay(rng, sp, wi);
                if (!s || math::isZero(s->weight)) {
                    break;
                }
                // Compute raster position for the primary ray
                if (length == 0) {
                    rasterPos = *scene->rasterPosition(s->wo, film_->aspectRatio());
                }

                // --------------------------------------------------------------------------------

                // Sample a NEE edge
                const bool nee = [&]() constexpr {
                    // Ignore NEE edge with naive direct sampling mode
                    if (ptMode_ == PTMode::Naive) {
                        return false;
                    }
                    // NEE edge can be samplable if current direction sampler
                    // (according to BSDF / phase) doesn't contain delta component.
                    if constexpr (sampleMode == SampleMode::Pixel) {
                        // Primary ray is not samplable via NEE in the pixel space sample mode
                        return length > 0 && !scene->isSpecular(s->sp);
                    }
                    else {
                        // Primary ray is samplable via NEE in the image space sample mode
                        return !scene->isSpecular(s->sp);
                    }
                }();
                if (nee) [&] {
                    // Sample a light
                    const auto sL = scene->sampleLight(rng, s->sp);
                    if (!sL) {
                        return;
                    }
                    if (!scene->visible(s->sp, sL->sp)) {
                        return;
                    }

                    // Recompute raster position for the primary edge
                    const auto rp = [&]() -> std::optional<Vec2> {
                        if (length == 0)
                            return scene->rasterPosition(-sL->wo, film_->aspectRatio());
                        else
                            return rasterPos;
                    }();
                    if (!rp) {
                        return;
                    }

                    // This light is samplable by direct strategy
                    // if the light doesn't contain delta component.
                    const bool directL = !scene->isSpecular(sL->sp);

                    // Evaluate and accumulate contribution
                    const auto wo = -sL->wo;
                    const auto fs = scene->evalContrb(s->sp, wi, wo);
                    const auto pdfSel = scene->pdfComp(s->sp, wi);
                    const auto misw = [&]() -> Float {
                        if (ptMode_ == PTMode::NEE) {
                            return 1_f;
                        }
                        if (!directL) {
                            return 1_f;
                        }
                        // Compute MIS weight only when wo can be sampled with both strategies.
                        return math::balanceHeuristic(
                            scene->pdfLight(s->sp, sL->sp, sL->wo), scene->pdf(s->sp, wi, wo));
                    }();
                    const auto C = throughput / pdfSel * fs * sL->weight * misw;
                    film_->splat(*rp, C);
                }();

                // --------------------------------------------------------------------------------

                // Intersection to next surface
                const auto hit = scene->intersect(s->ray());
                if (!hit) {
                    break;
                }

                // --------------------------------------------------------------------------------

                // Update throughput
                throughput *= s->weight;

                // --------------------------------------------------------------------------------

                // Accumulate contribution from light
                const bool direct = [&]() -> bool {
                    // Direct strategy is samplable if the ray hit with light
                    if (ptMode_ == PTMode::NEE) {
                        // In NEE mode, use direct strategy only when a NEE edge cannot be sampled.
                        return !nee && scene->isLight(*hit);
                    }
                    else {
                        return scene->isLight(*hit);
                    }
                }();
                if (direct) {
                    const auto woL = -s->wo;
                    const auto fs = scene->evalContrbEndpoint(*hit, woL);
                    const auto misw = [&]() -> Float {
                        if (ptMode_ == PTMode::Naive) {
                            return 1_f;
                        }
                        if (!nee) {
                            return 1_f;
                        }
                        // The continuation edge can be sampled via both direct and NEE
                        return math::balanceHeuristic(
                            scene->pdf(s->sp, wi, s->wo), scene->pdfLight(s->sp, *hit, woL));
                    }();
                    const auto C = throughput * fs * misw;
                    film_->splat(rasterPos, C);
                }

                // --------------------------------------------------------------------------------

                // Russian roulette
                if (length > 3) {
                    const auto q = glm::max(.2_f, 1_f - glm::compMax(throughput));
                    if (rng.u() < q) {
                        break;
                    }
                    throughput /= 1_f - q;
                }

                // --------------------------------------------------------------------------------

                // Update
                wi = -s->wo;
                sp = *hit;
            }
        });

        // ----------------------------------------------------------------------------------------
        
        // Rescale film
        if constexpr (sampleMode == SampleMode::Pixel) {
            film_->rescale(1_f / processed);
        }
        else {
            film_->rescale(Float(size.w * size.h) / processed);
        }
    }
};

using Renderer_PT_Pixel = Renderer_PT<SampleMode, SampleMode::Pixel>;
LM_COMP_REG_IMPL(Renderer_PT_Pixel, "renderer::pt");
using Renderer_PT_Image = Renderer_PT<SampleMode, SampleMode::Image>;
LM_COMP_REG_IMPL(Renderer_PT_Image, "renderer::pt::image");

LM_NAMESPACE_END(LM_NAMESPACE)
