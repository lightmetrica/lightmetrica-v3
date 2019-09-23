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
#include <lm/debug.h>

#define VOLPT_IMAGE_SAMPLNG 0

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Renderer_VolPTNaive final : public Renderer {
private:
    Film* film_;
    int maxLength_;
    Float rrProb_;
    std::optional<unsigned int> seed_;
    Component::Ptr<scheduler::Scheduler> sched_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(film_, maxLength_, rrProb_, sched_);
    }

    virtual void foreachUnderlying(const ComponentVisitor& visit) override {
        comp::visit(visit, film_);
        comp::visit(visit, sched_);
    }

public:
    virtual bool construct(const Json& prop) override {
        film_ = json::compRef<Film>(prop, "output");
        maxLength_ = json::value<int>(prop, "max_length");
        rrProb_ = json::value<Float>(prop, "rr_prob", .2_f);
        const auto schedName = json::value<std::string>(prop, "scheduler");
        seed_ = json::valueOrNone<unsigned int>(prop, "seed");
#if VOLPT_IMAGE_SAMPLNG
        sched_ = comp::create<scheduler::Scheduler>(
            "scheduler::spi::" + schedName, makeLoc("scheduler"), prop);
#else
        sched_ = comp::create<scheduler::Scheduler>(
            "scheduler::spp::" + schedName, makeLoc("scheduler"), prop);
#endif
        return true;
    }

    virtual void render(const Scene* scene) const override {
        film_->clear();
        const auto size = film_->size();
        const auto processed = sched_->run([&](long long pixelIndex, long long, int threadid) {
            // Per-thread random number generator
            thread_local Rng rng(seed_ ? *seed_ + threadid : math::rngSeed());

#if VOLPT_IMAGE_SAMPLNG
            LM_UNUSED(pixelIndex);
            const Vec4 window(0_f, 0_f, 1_f, 1_f);
#else
            // Pixel positions
            const int x = int(pixelIndex % size.w);
            const int y = int(pixelIndex / size.w);
            const auto dx = 1_f / size.w;
            const auto dy = 1_f / size.h;
            const Vec4 window(dx * x, dy * y, dx, dy);
#endif

            //if (x == 70 && y == 16) {
            //    __debugbreak();
            //}
            //
            // Path throughput
            Vec3 throughput(1_f);

            // Initial sampleRay function
            std::function<std::optional<RaySample>()> sampleRay = [&]() {
                return scene->samplePrimaryRay(rng, window, film_->aspectRatio());
            };

            // Perform random walk
            Vec3 L(0_f);
            Vec2 rasterPos{};
            for (int length = 0; length < maxLength_; length++) {
                #if LM_DEBUG_MODE
                if (x == 70 && y == 16) {
                    __debugbreak();
                }
                #endif

                // Sample a ray
                const auto s = sampleRay();
                if (!s || math::isZero(s->weight)) {
                    break;
                }

                // Compute raster position for the primary ray
                if (length == 0) {
                    rasterPos = *scene->rasterPosition(s->wo, film_->aspectRatio());
                }

                // Sample next scene interaction
                const auto sd = scene->sampleDistance(rng, s->sp, s->wo);
                if (!sd) {
                    break;
                }

                // Update throughput
                throughput *= s->weight * sd->weight;

                if (x == 70 && y == 16) {
                    debug::pollFloat("throughput", glm::compMax(throughput));
                }

                // Accumulate contribution from emissive interaction
                if (scene->isLight(sd->sp)) {
                    const auto C = throughput * scene->evalContrbEndpoint(sd->sp, -s->wo);
                    L += C;
                }

                // Russian roulette
                if (length > 3) {
                    const auto q = glm::max(rrProb_, 1_f - glm::compMax(throughput));
                    if (rng.u() < q) {
                        break;
                    }
                    throughput /= 1_f - q;
                }

                // Update
                sampleRay = [scene, wi = -s->wo, sp = sd->sp]() {
                    return scene->sampleRay(rng, sp, wi);
                };
            }

            //if (glm::compMax(L) > 1e+20) {
            //    LM_INFO("{} {}", x, y);
            //    __debugbreak();
            //}

            // Accumulate contribution
            film_->splat(rasterPos, L);
        });

#if VOLPT_IMAGE_SAMPLNG
        film_->rescale(Float(size.w * size.h) / processed);
#else
        film_->rescale(1_f / processed);
#endif
    }
};

LM_COMP_REG_IMPL(Renderer_VolPTNaive, "renderer::volpt_naive");

LM_NAMESPACE_END(LM_NAMESPACE)
