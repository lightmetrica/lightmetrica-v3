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

#define VOLPT_DEBUG_VIS 0
#define VOLPT_IMAGE_SAMPLNG 0

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Renderer_VolPT final : public Renderer {
private:
    Film* film_;
    int maxLength_;
    Float rrProb_;
    std::optional<unsigned int> seed_;
    Component::Ptr<scheduler::Scheduler> sched_;

    #if VOLPT_DEBUG_VIS
    mutable std::vector<Ray> sampledRays_;
    #endif

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(film_, maxLength_, rrProb_, sched_);
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
        seed_ = json::valueOrNone<unsigned int>(prop, "seed");
        rrProb_ = json::value<Float>(prop, "rr_prob", .2_f);
        const auto schedName = json::value<std::string>(prop, "scheduler");
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
            const auto dx = 1_f/size.w;
            const auto dy = 1_f/size.h;
            const Vec4 window(dx*x, dy*y, dx, dy);
#endif

            // Incident ray direction
            Vec3 wi{};

            // Current scene interaction
            auto sp = SceneInteraction::makeCameraTerminator(window, film_->aspectRatio());

            // Path throughput
            Vec3 throughput(1_f);

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
#if VOLPT_IMAGE_SAMPLNG
                const bool nee = !scene->isSpecular(s->sp);
#else
                const bool nee = length > 0 && !scene->isSpecular(s->sp);
#endif
                if (nee) [&] {
                    // Sample a light
                    const auto sL = scene->sampleLight(rng, s->sp);
                    if (!sL) {
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
                    const auto pdfSel = scene->pdfComp(s->sp, wi);
                    const auto C = throughput / pdfSel * *Tr * fs * sL->weight;
                    film_->splat(*rp, C);
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
                    film_->splat(rasterPos, C);
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
                wi = -s->wo;
                sp = sd->sp;
            }
        });

#if VOLPT_IMAGE_SAMPLNG
        film_->rescale(Float(size.w* size.h) / processed);
#else
        film_->rescale(1_f / processed);
#endif
    }
};

LM_COMP_REG_IMPL(Renderer_VolPT, "renderer::volpt");

LM_NAMESPACE_END(LM_NAMESPACE)
