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
#include <lm/path.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// Base of path tracing renderers
class Renderer_PT_Base : public Renderer {
protected:
    Scene* scene_;                                  // Reference to scene asset
    Film* film_;                                    // Reference to film asset for output
    int max_length_;                                // Maximum number of path length
    std::optional<unsigned int> seed_;              // Random seed
    Component::Ptr<scheduler::Scheduler> sched_;    // Scheduler for parallel processing

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(scene_, film_, max_length_, seed_, sched_);
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
        scene_->camera()->set_aspect_ratio(film_->aspect());
        max_length_ = json::value<int>(prop, "max_length");
        seed_ = json::value_or_none<unsigned int>(prop, "seed");
        
        // Scheduler is fixed to image space scheduler
        const auto sched_name = json::value<std::string>(prop, "scheduler");
        sched_ = comp::create<scheduler::Scheduler>(
            "scheduler::spi::" + sched_name, make_loc("scheduler"), prop);
    }
};

// ------------------------------------------------------------------------------------------------

// Path tracing with BSDF sampling
class Renderer_PT_Naive final : public Renderer_PT_Base {
public:
    virtual Json render() const override {
        scene_->require_renderable();

        // Clear film
        film_->clear();
        const auto size = film_->size();

        // Execute parallel process
        const auto processed = sched_->run([&](long long, long long, int threadid) {
            // Per-thread random number generator
            thread_local Rng rng(seed_ ? *seed_ + threadid : math::rng_seed());

            // Path throughput
            Vec3 throughput(1_f);

            // Incident direction and current scene interaction
            Vec3 wi = {};
            auto sp = SceneInteraction::make_camera_term();

            // Raster position
            Vec2 raster_pos{};

            // Perform random walk
            for (int length = 0; length < max_length_; length++) {
                // Sample a ray based on current scene interaction
                const auto s = path::sample_ray(rng.next<path::RaySampleU>(), scene_, sp, wi, TransDir::EL);
                if (!s || math::is_zero(s->weight)) {
                    break;
                }

                // Compute raster position for the primary ray
                if (length == 0) {
                    raster_pos = *path::raster_position(scene_, s->wo);
                }

                // Intersection to next surface
                const auto hit = scene_->intersect(s->ray());
                if (!hit) {
                    break;
                }

                // Update throughput
                throughput *= s->weight;

                // Accumulate contribution from light
                if (scene_->is_light(*hit)) {
                    const auto spL = hit->as_type(SceneInteraction::LightEndpoint);
                    const auto woL = -s->wo;
                    const auto Le = path::eval_contrb_direction(scene_, spL, {}, woL, TransDir::LE, {});
                    const auto C = throughput * Le;
                    film_->splat(raster_pos, C);
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
        film_->rescale(Float(size.w * size.h) / processed);

        return { {"processed", processed} };
    }
};

LM_COMP_REG_IMPL(Renderer_PT_Naive, "renderer::pt_naive");

// ------------------------------------------------------------------------------------------------

// Path tracing with NEE
class Renderer_PT_NEE final : public Renderer_PT_Base {
public:
    virtual Json render() const override {
        scene_->require_renderable();

        // Clear film
        film_->clear();
        const auto size = film_->size();

        // Execute parallel process
        const auto processed = sched_->run([&](long long, long long, int threadid) {
            // Per-thread random number generator
            thread_local Rng rng(seed_ ? *seed_ + threadid : math::rng_seed());

            // Path throughput
            Vec3 throughput(1_f);

            // Incident direction and current scene interaction
            Vec3 wi = {};
            auto sp = SceneInteraction::make_camera_term();

            // Raster position
            Vec2 raster_pos{};

            // Perform random walk
            for (int length = 0; length < max_length_; length++) {
                // Sample a ray based on current scene interaction
                const auto s = path::sample_ray(rng.next<path::RaySampleU>(), scene_, sp, wi, TransDir::EL);
                if (!s || math::is_zero(s->weight)) {
                    break;
                }

                // Compute raster position for the primary ray
                if (length == 0) {
                    raster_pos = *path::raster_position(scene_, s->wo);
                }

                // Sample a NEE edge
                [&]{
                    // Sample a light
                    const auto sL = path::sample_direct_light(rng.next<path::RaySampleU>(), scene_, s->sp);
                    if (!sL) {
                        return;
                    }
                    if (!scene_->visible(s->sp, sL->sp)) {
                        return;
                    }

                    // Recompute raster position for the primary edge
                    Vec2 rp = raster_pos;
                    if (length == 0) {
                        const auto rp_ = path::raster_position(scene_ , -sL->wo);
                        if (!rp_) {
                            return;
                        }
                        rp = *rp_;
                    }

                    // Evaluate and accumulate contribution
                    const auto wo = -sL->wo;
                    const auto fs = path::eval_contrb_direction(scene_, s->sp, wi, wo, TransDir::EL, true);
                    const auto C = throughput * fs * sL->weight;
                    film_->splat(rp, C);
                }();

                // Intersection to next surface
                const auto hit = scene_->intersect(s->ray());
                if (!hit) {
                    break;
                }

                // Update throughput
                throughput *= s->weight;

                // Accumulate contribution from light only when a NEE edge is not samplable.
                // This happens when the direction is sampled from delta distribution.
                // Note that this include the case where the surface has multi-component material
                // containing delta component and the direction is sampled from the component.
                const auto not_samplable_by_nee = s->specular;
                if (not_samplable_by_nee && scene_->is_light(*hit)) {
                    const auto spL = hit->as_type(SceneInteraction::LightEndpoint);
                    const auto woL = -s->wo;
                    const auto Le = path::eval_contrb_direction(scene_, spL, {}, woL, TransDir::LE, {});
                    const auto C = throughput * Le;
                    film_->splat(raster_pos, C);
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
        film_->rescale(Float(size.w * size.h) / processed);

        return { {"processed", processed} };
    }
};

LM_COMP_REG_IMPL(Renderer_PT_NEE, "renderer::pt_nee");

// ------------------------------------------------------------------------------------------------

// Path tracing with MIS
class Renderer_PT_MIS final : public Renderer_PT_Base {
public:
    virtual Json render() const override {
        scene_->require_renderable();

        // Clear film
        film_->clear();
        const auto size = film_->size();

        // Execute parallel process
        const auto processed = sched_->run([&](long long, long long, int threadid) {
            // Per-thread random number generator
            thread_local Rng rng(seed_ ? *seed_ + threadid : math::rng_seed());

            // Path throughput
            Vec3 throughput(1_f);

            // Incident direction and current scene interaction
            Vec3 wi = {};
            auto sp = SceneInteraction::make_camera_term();

            // Raster position
            Vec2 raster_pos{};

            // Perform random walk
            for (int length = 0; length < max_length_; length++) {
                // Sample a ray based on current scene interaction
                const auto s = path::sample_ray(rng.next<path::RaySampleU>(), scene_, sp, wi, TransDir::EL);
                if (!s || math::is_zero(s->weight)) {
                    break;
                }

                // Compute raster position for the primary ray
                if (length == 0) {
                    raster_pos = *path::raster_position(scene_, s->wo);
                }

                // Sample a NEE edge
                [&] {
                    // Sample a light
                    const auto sL = path::sample_direct_light(rng.next<path::RaySampleU>(), scene_, s->sp);
                    if (!sL) {
                        return;
                    }
                    if (!scene_->visible(s->sp, sL->sp)) {
                        return;
                    }

                    // Recompute raster position for the primary edge
                    Vec2 rp = raster_pos;
                    if (length == 0) {
                        const auto rp_ = path::raster_position(scene_, -sL->wo);
                        if (!rp_) {
                            return;
                        }
                        rp = *rp_;
                    }

                    // Evaluate and accumulate contribution
                    const auto wo = -sL->wo;
                    const auto fs = path::eval_contrb_direction(scene_, s->sp, wi, wo, TransDir::EL, true);
                    const auto mis_w = [&]() -> Float {
                        // When the light is not samplable by BSDF sampling, we will use only NEE.
                        // This includes, for instance, the light sampling for
                        // directional light, environment light, point light, etc.
                        const bool is_samplable_by_bsdf_sampling = !s->specular && !sL->sp.geom.degenerated;
                        if (!is_samplable_by_bsdf_sampling) {
                            return 1_f;
                        }

                        // MIS weight using balance heuristic
                        const auto pdf_light = path::pdf_direct(scene_, s->sp, sL->sp, sL->wo);
                        const auto pdf_bsdf = path::pdf_direction(scene_, s->sp, wi, wo, true);
                        return math::balance_heuristic(pdf_light, pdf_bsdf);
                    }();
                    const auto C = throughput * fs * sL->weight * mis_w;
                    film_->splat(rp, C);
                }();

                // Intersection to next surface
                const auto hit = scene_->intersect(s->ray());
                if (!hit) {
                    break;
                }

                // Update throughput
                throughput *= s->weight;

                // Accumulate contribution from light 
                if (scene_->is_light(*hit)) {
                    const auto spL = hit->as_type(SceneInteraction::LightEndpoint);
                    const auto woL = -s->wo;
                    const auto fs = path::eval_contrb_direction(scene_, spL, {}, woL, TransDir::LE, {});
                    const auto mis_w = [&]() -> Float {
                        // If the light is not samplable by NEE, we will use only BSDF sampling.
                        const auto is_samplable_by_nee = !s->specular;
                        if (!is_samplable_by_nee) {
                            return 1_f;
                        }

                        // MIS weight using balance heuristic
                        const auto pdf_bsdf = path::pdf_direction(scene_, s->sp, wi, s->wo, true);
                        const auto pdf_light = path::pdf_direct(scene_, s->sp, spL, woL);
                        return math::balance_heuristic(pdf_bsdf, pdf_light);
                    }();
                    const auto C = throughput * fs * mis_w;
                    film_->splat(raster_pos, C);
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
        film_->rescale(Float(size.w * size.h) / processed);

        return { {"processed", processed} };
    }
};

LM_COMP_REG_IMPL(Renderer_PT_MIS, "renderer::pt_mis");

LM_NAMESPACE_END(LM_NAMESPACE)
