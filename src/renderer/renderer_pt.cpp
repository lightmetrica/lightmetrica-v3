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
        ptMode_ = [&]() -> PTMode {
            const auto s = json::value<std::string>(prop, "mode", "mis");
            if (s == "naive")
                return PTMode::Naive;
            else if (s == "nee")
                return PTMode::NEE;
            else if (s == "mis")
                return PTMode::MIS;
            LM_UNREACHABLE_RETURN();
        }();
        const auto schedName = json::value<std::string>(prop, "scheduler");
        if constexpr (sampleMode == SampleMode::Pixel)
            sched_ = comp::create<scheduler::Scheduler>(
                "scheduler::spp::" + schedName, makeLoc("scheduler"), prop);
        else
            sched_ = comp::create<scheduler::Scheduler>(
                "scheduler::spi::" + schedName, makeLoc("scheduler"), prop);
        return true;
    }

    virtual void render(const Scene* scene) const override {
        film_->clear();
        const auto size = film_->size();
        const auto processed = sched_->run([&](long long pixelIndex, long long, int) {
            // Per-thread random number generator
            thread_local Rng rng;

            // Sample window
            const auto window = [&]() -> Vec4 {
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

            // Path throughput
            Vec3 throughput(1_f);

            // Incident direction and current surface point
            Vec3 wi = {};
            auto sp = SceneInteraction::makeCameraTerminator(window, film_->aspectRatio());

            // Perform random walk
            Vec2 rasterPos{};
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

                // Sample a NEE edge
                const bool nee = [&]() {
                    // Primary ray is samplable via NEE in the image space sample mode
                    if constexpr (sampleMode == SampleMode::Pixel)
                        return length > 0 && !scene->isSpecular(s->sp);
                    else
                        return !scene->isSpecular(s->sp);
                }();
                if (ptMode_ != PTMode::Naive && nee) [&] {
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

                    // Evaluate and accumulate contribution
                    const auto wo = -sL->wo;
                    const auto fs = scene->evalContrb(s->sp, wi, wo);
                    const auto pdfSel = scene->pdfComp(s->sp, wi);
                    const auto misw = ptMode_ == PTMode::NEE
                        ? 1_f
                        : math::balanceHeuristic(
                            scene->pdfLight(s->sp, sL->sp, sL->wo), scene->pdf(s->sp, wi, wo));
                    const auto C = throughput / pdfSel * fs * sL->weight * misw;
                    film_->splat(*rp, C);
                }();

                // Intersection to next surface
                const auto hit = scene->intersect(s->ray());
                if (!hit) {
                    break;
                }

                // Update throughput
                throughput *= s->weight;

                // Accumulate contribution from light
                // In NEE mode, only use direct strategy when a NEE edge cannot be sampled
                const bool direct = (ptMode_ != PTMode::NEE) || (ptMode_ == PTMode::NEE && !nee);
                if (direct && scene->isLight(*hit)) {
                    const auto woL = -s->wo;
                    const auto fs = scene->evalContrbEndpoint(*hit, woL);
                    const auto misw = ptMode_ == PTMode::Naive || !nee
                        ? 1_f 
                        : math::balanceHeuristic(
                            scene->pdf(s->sp, wi, s->wo), scene->pdfLight(s->sp, *hit, woL));
                    const auto C = throughput * fs * misw;
                    film_->splat(rasterPos, C);
                }

                // Russian roulette
                if (length > 3) {
                    const auto q = glm::max(.2_f, 1_f - glm::compMax(throughput));
                    if (rng.u() < q) {
                        break;
                    }
                    throughput /= 1_f - q;
                }

                // Update
                wi = -s->wo;
                sp = *hit;
            }
        });
        
        // Rescale film
        if constexpr (sampleMode == SampleMode::Pixel)
            film_->rescale(1_f / processed);
        else
            film_->rescale(Float(size.w * size.h) / processed);
    }
};

using Renderer_PT_Pixel = Renderer_PT<SampleMode, SampleMode::Pixel>;
LM_COMP_REG_IMPL(Renderer_PT_Pixel, "renderer::pt");
using Renderer_PT_Image = Renderer_PT<SampleMode, SampleMode::Image>;
LM_COMP_REG_IMPL(Renderer_PT_Image, "renderer::pt::image");

LM_NAMESPACE_END(LM_NAMESPACE)
