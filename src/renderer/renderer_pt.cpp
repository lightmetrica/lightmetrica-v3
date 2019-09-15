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

class Renderer_PT final : public Renderer {
private:
    Film* film_;
    int maxLength_;
    Component::Ptr<scheduler::SPPScheduler> sched_;
    PTMode mode_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(film_, maxLength_, sched_, mode_);
    }

    virtual void foreachUnderlying(const ComponentVisitor& visit) override {
        comp::visit(visit, film_);
        comp::visit(visit, sched_);
    }

public:
    virtual bool construct(const Json& prop) override {
        film_ = json::compRef<Film>(prop, "output");
        maxLength_ = json::value<int>(prop, "max_length");
        const auto schedName = json::value<std::string>(prop, "scheduler");
        sched_ = comp::create<scheduler::SPPScheduler>(
            "sppscheduler::" + schedName, makeLoc("sampler"), prop);
        mode_ = [&]() -> PTMode {
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
        return true;
    }

    virtual void render(const Scene* scene) const override {
        film_->clear();
        const auto [w, h] = film_->size();
        const auto processed = sched_->run(film_->numPixels(), [&](long long pixelIndex, long long sampleIndex, int) {
            // Per-thread random number generator
            thread_local Rng rng;

            // Pixel positions
            const int x = int(pixelIndex % w);
            const int y = int(pixelIndex / w);

            // Path throughput
            Vec3 throughput(1_f);

            // Incident direction and current surface point
            Vec3 wi = {};
            SceneInteraction sp;

            // Initial sampleRay function
            std::function<std::optional<RaySample>()> sampleRay = [&]() {
                Float dx = 1_f/w, dy = 1_f/h;
                return scene->samplePrimaryRay(rng, {dx*x, dy*y, dx, dy}, film_->aspectRatio());
            };

            // Perform random walk
            Vec3 L(0_f);
            for (int length = 0; length < maxLength_; length++) {
                // Sample a ray
                const auto s = sampleRay();
                if (!s || math::isZero(s->weight)) {
                    break;
                }

                // Sample a NEE edge
                const bool nee = length > 0 && !scene->isSpecular(s->sp);
                if (mode_ != PTMode::Naive && nee) [&] {
                    // Sample a light
                    const auto sL = scene->sampleLight(rng, s->sp);
                    if (!sL) {
                        return;
                    }
                    if (!scene->visible(s->sp, sL->sp)) {
                        return;
                    }
                    // Evaluate and accumulate contribution
                    const auto wo = -sL->wo;
                    const auto fs = scene->evalContrb(s->sp, wi, wo);
                    const auto pdfSel = scene->pdfComp(s->sp, wi);
                    const auto misw = mode_ == PTMode::NEE
                        ? 1_f
                        : math::balanceHeuristic(
                            scene->pdfLight(s->sp, sL->sp, sL->wo), scene->pdf(s->sp, wi, wo));
                    const auto C = throughput / pdfSel * fs * sL->weight * misw;
                    L += C;
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
                const bool direct = (mode_ != PTMode::NEE) || (mode_ == PTMode::NEE && !nee);
                if (direct && scene->isLight(*hit)) {
                    const auto woL = -s->wo;
                    const auto fs = scene->evalContrbEndpoint(*hit, woL);
                    const auto misw = mode_ == PTMode::Naive || !nee
                        ? 1_f 
                        : math::balanceHeuristic(
                            scene->pdf(s->sp, wi, s->wo), scene->pdfLight(s->sp, *hit, woL));
                    const auto C = throughput * fs * misw;
                    L += C;
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
                sampleRay = [&]() {
                    return scene->sampleRay(rng, sp, wi);
                };
            }

            // Accumulate contribution
            film_->incAve(x, y, sampleIndex, L);
        });
    }
};

LM_COMP_REG_IMPL(Renderer_PT, "renderer::pt");

LM_NAMESPACE_END(LM_NAMESPACE)
