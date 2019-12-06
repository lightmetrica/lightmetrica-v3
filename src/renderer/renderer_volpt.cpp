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
    Scene* scene_;
    Film* film_;
    int max_length_;
    Float rr_prob_;
    std::optional<unsigned int> seed_;
    Component::Ptr<scheduler::Scheduler> sched_;

    #if VOLPT_DEBUG_VIS
    mutable std::vector<Ray> sampledRays_;
    #endif

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(scene_, film_, max_length_, rr_prob_, sched_);
        #if VOLPT_DEBUG_VIS
        ar(sampledRays_);
        #endif
    }

    virtual void foreach_underlying(const ComponentVisitor& visit) override {
        comp::visit(visit, scene_);
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
    virtual void construct(const Json& prop) override {
        scene_ = json::comp_ref<Scene>(prop, "scene");
        film_ = json::comp_ref<Film>(prop, "output");
        max_length_ = json::value<int>(prop, "max_length");
        seed_ = json::value_or_none<unsigned int>(prop, "seed");
        rr_prob_ = json::value<Float>(prop, "rr_prob", .2_f);
        const auto sched_name = json::value<std::string>(prop, "scheduler");
#if VOLPT_IMAGE_SAMPLNG
        sched_ = comp::create<scheduler::Scheduler>(
            "scheduler::spi::" + sched_name, makeLoc("scheduler"), prop);
#else
        sched_ = comp::create<scheduler::Scheduler>(
            "scheduler::spp::" + sched_name, make_loc("scheduler"), prop);
#endif
    }

    virtual void render() const override {
		scene_->require_renderable();

        film_->clear();
        const auto size = film_->size();
        const auto processed = sched_->run([&](long long pixel_index, long long, int threadid) {
            // Per-thread random number generator
            thread_local Rng rng(seed_ ? *seed_ + threadid : math::rng_seed());

#if VOLPT_IMAGE_SAMPLNG
            LM_UNUSED(pixelIndex);
            const Vec4 window(0_f, 0_f, 1_f, 1_f);
#else
            // Pixel positions
            const int x = int(pixel_index % size.w);
            const int y = int(pixel_index / size.w);
            const auto dx = 1_f/size.w;
            const auto dy = 1_f/size.h;
            const Vec4 window(dx*x, dy*y, dx, dy);
#endif

            // Incident ray direction
            Vec3 wi{};

            // Current scene interaction
            auto sp = SceneInteraction::make_camera_terminator(window, film_->aspect_ratio());

            // Path throughput
            Vec3 throughput(1_f);

            // Perform random walk
            Vec2 raster_pos{};
            for (int length = 0; length < max_length_; length++) {
                // Sample a ray
                const auto s = scene_->sample_ray(rng, sp, wi);
                if (!s || math::is_zero(s->weight)) {
                    break;
                }

                // Compute raster position for the primary ray
                if (length == 0) {
                    raster_pos = *scene_->raster_position(s->wo, film_->aspect_ratio());
                }

                // Sample a NEE edge
#if VOLPT_IMAGE_SAMPLNG
                const bool nee = !scene->is_specular(s->sp);
#else
                const bool nee = length > 0 && !scene_->is_specular(s->sp, s->comp);
#endif
                if (nee) [&] {
                    // Sample a light
                    const auto sL = scene_->sample_direct_light(rng, s->sp);
                    if (!sL) {
                        return;
                    }

                    // Recompute raster position for the primary edge
                    const auto rp = [&]() -> std::optional<Vec2> {
                        if (length == 0)
                            return scene_->raster_position(-sL->wo, film_->aspect_ratio());
                        else
                            return raster_pos;
                    }();
                    if (!rp) {
                        return;
                    }
                    
                    // Transmittance
                    const auto Tr = scene_->eval_transmittance(rng, s->sp, sL->sp);
                    if (math::is_zero(Tr)) {
                        return;
                    }

                    // Evaluate and accumulate contribution
                    const auto wo = -sL->wo;
                    const auto fs = scene_->eval_contrb(s->sp, s->comp, wi, wo);
                    const auto pdf_sel = scene_->pdf_comp(s->sp, s->comp, wi);
                    const auto C = throughput / pdf_sel * Tr * fs * sL->weight;
                    film_->splat(*rp, C);
                }();

                // Sample next scene interaction
                const auto sd = scene_->sample_distance(rng, s->sp, s->wo);
                if (!sd) {
                    break;
                }

                // Update throughput
                throughput *= s->weight * sd->weight;

                // Accumulate contribution from emissive interaction
                if (!nee && scene_->is_light(sd->sp)) {
                    const auto C = throughput * scene_->eval_contrb_endpoint(sd->sp, -s->wo);
                    film_->splat(raster_pos, C);
                }

                // Russian roulette
                if (length > 3) {
                    const auto q = glm::max(rr_prob_, 1_f - glm::compMax(throughput));
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
