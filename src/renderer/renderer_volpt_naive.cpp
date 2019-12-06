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
    Scene* scene_;
    Film* film_;
    int max_length_;
    Float rr_prob_;
    std::optional<unsigned int> seed_;
    Component::Ptr<scheduler::Scheduler> sched_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(scene_, film_, max_length_, rr_prob_, sched_);
    }

    virtual void foreach_underlying(const ComponentVisitor& visit) override {
        comp::visit(visit, scene_);
        comp::visit(visit, film_);
        comp::visit(visit, sched_);
    }

public:
    virtual void construct(const Json& prop) override {
        scene_ = json::comp_ref<Scene>(prop, "scene");
        film_ = json::comp_ref<Film>(prop, "output");
        max_length_ = json::value<int>(prop, "max_length");
        rr_prob_ = json::value<Float>(prop, "rr_prob", .2_f);
        const auto sched_name = json::value<std::string>(prop, "scheduler");
        seed_ = json::value_or_none<unsigned int>(prop, "seed");
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
            const auto dx = 1_f / size.w;
            const auto dy = 1_f / size.h;
            const Vec4 window(dx * x, dy * y, dx, dy);
#endif

            // Path throughput
            Vec3 throughput(1_f);

            // Incident direction and current surface point
            Vec3 wi = {};
            auto sp = SceneInteraction::make_camera_terminator(window, film_->aspect_ratio());

            // Perform random walk
            Vec3 L(0_f);
            Vec2 rasterPos{};
            for (int length = 0; length < max_length_; length++) {
                // Sample a ray
                const auto s = scene_->sample_ray(rng, sp, wi);
                if (!s || math::is_zero(s->weight)) {
                    break;
                }

                // Compute raster position for the primary ray
                if (length == 0) {
                    rasterPos = *scene_->raster_position(s->wo, film_->aspect_ratio());
                }

                // Sample next scene interaction
                const auto sd = scene_->sample_distance(rng, s->sp, s->wo);
                if (!sd) {
                    break;
                }

                // Update throughput
                throughput *= s->weight * sd->weight;

                if (x == 70 && y == 16) {
                    debug::poll_float("throughput", glm::compMax(throughput));
                }

                // Accumulate contribution from emissive interaction
                if (scene_->is_light(sd->sp)) {
                    const auto C = throughput * scene_->eval_contrb_endpoint(sd->sp, -s->wo);
                    L += C;
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
