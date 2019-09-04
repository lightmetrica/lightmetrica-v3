/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/user.h>
#include <lm/renderer.h>
#include <lm/scene.h>
#include <lm/film.h>
#include <lm/scheduler.h>
#include <lm/serial.h>
#include <lm/json.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Renderer_VolPTNaive final : public Renderer {
private:
    Film* film_;
    int maxLength_;
    Component::Ptr<scheduler::SPPScheduler> sched_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(film_, maxLength_, sched_);
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

                // Sample next scene interaction
                const auto sd = scene->sampleDistance(rng, s->sp, s->wo);
                if (!sd) {
                    break;
                }

                // Update throughput
                throughput *= s->weight * sd->weight;

                // Accumulate contribution from emissive interaction
                if (scene->isLight(sd->sp)) {
                    const auto C = throughput * scene->evalContrbEndpoint(sd->sp, -s->wo);
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
                sampleRay = [&, wi = -s->wo, sp = sd->sp]() {
                    return scene->sampleRay(rng, sp, wi);
                };
            }

            // Accumulate contribution
            film_->incAve(x, y, sampleIndex, L);
        });
    }
};

LM_COMP_REG_IMPL(Renderer_VolPTNaive, "renderer::volpt_naive");

LM_NAMESPACE_END(LM_NAMESPACE)
