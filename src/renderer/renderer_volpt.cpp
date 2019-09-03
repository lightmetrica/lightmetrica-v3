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

#define VOLPT_DEBUG_VIS 0

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Renderer_VolPT final : public Renderer {
private:
    Film* film_;
    int maxLength_;
    Component::Ptr<scheduler::SPPScheduler> sched_;

    #if VOLPT_DEBUG_VIS
    mutable std::vector<Ray> sampledRays_;
    #endif

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(film_, maxLength_, sched_);
        #if VOLPT_DEBUG_VIS
        ar(sampledRays_);
        #endif
    }

    virtual void foreachUnderlying(const ComponentVisitor& visit) override {
        comp::visit(visit, film_);
        comp::visit(visit, sched_);
    }

    #if VOLPT_DEBUG_VIS
    virtual void* underlyingRawPointer(const std::string& query) const override {
        if (query == "sampledRays") {
            return &sampledRays_;
        }
        return nullptr;
    }
    #endif

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

            // Incident ray direction
            Vec3 wi;

            // Path throughput
            Vec3 throughput(1_f);

            // Initial sampleRay function
            std::function<std::optional<RaySample>()> sampleRay = [&]() {
                Float dx = 1_f/w, dy = 1_f/h;
                return scene->samplePrimaryRay(rng, {dx*x, dy*y, dx, dy}, film_->aspectRatio());
            };

            // Perform random walk
            for (int length = 0; length < maxLength_; length++) {
                // Sample a ray
                const auto s = sampleRay();
                if (!s || math::isZero(s->weight)) {
                    break;
                }

                // Sample a NEE edge
                const bool nee = length > 0 && !scene->isSpecular(s->sp);
                if (nee) [&] {
                    // Sample a light
                    const auto sL = scene->sampleLight(rng, s->sp);
                    if (!sL) {
                        return;
                    }
                    
                    // Transmittance
                    const auto Tr = scene->evalTransmittance(rng, s->sp, sL->sp);
                    if (!Tr) {
                        return;
                    }

                    #if VOLPT_DEBUG_VIS
                    const bool record = 500 < x && x < 600 && 500 < y && y < 600;
                    if (threadId == 0 && sampledRays_.size() < 1000 && record) {
                        sampledRays_.push_back({ s->sp.geom.p, -sL->wo });
                    }
                    #endif

                    // Evaluate and accumulate contribution
                    const auto wo = -sL->wo;
                    const auto fs = scene->evalContrb(s->sp, wi, wo);
                    const auto C = throughput * *Tr * fs * sL->weight;
                    film_->incAve(x, y, sampleIndex, C);
                }();

                // Sample next scene interaction
                const auto sd = scene->sampleDistance(rng, s->sp, s->wo);
                if (!sd) {
                    break;
                }

                // Update throughput
                throughput *= s->weight * sd->weight;

                // Accumulate contribution from emissive interaction
                if (!nee && scene->isLight(sd->sp)) {
                    const auto C = throughput * scene->evalContrbEndpoint(sd->sp, -s->wo);
                    film_->incAve(x, y, sampleIndex, C);
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
                sampleRay = [&, sp = sd->sp]() {
                    return scene->sampleRay(rng, sp, wi);
                };
            }
        });
    }
};

LM_COMP_REG_IMPL(Renderer_VolPT, "renderer::volpt");

LM_NAMESPACE_END(LM_NAMESPACE)
