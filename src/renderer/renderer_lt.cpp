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

// Light trancing with NEE
class Renderer_LT_NEE : public Renderer {
private:
    Scene* scene_;                                  // Reference to scene asset
    Film* film_;                                    // Reference to film asset for output
    int max_verts_;                                 // Maximum number of path vertices
    std::optional<unsigned int> seed_;              // Random seed
    Component::Ptr<scheduler::Scheduler> sched_;    // Scheduler for parallel processing

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(scene_, film_, max_verts_, seed_, sched_);
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
        max_verts_ = json::value<int>(prop, "max_verts");
        seed_ = json::value_or_none<unsigned int>(prop, "seed");

        // Scheduler is fixed to image space scheduler
        const auto sched_name = json::value<std::string>(prop, "scheduler");
        sched_ = comp::create<scheduler::Scheduler>(
            "scheduler::spi::" + sched_name, make_loc("scheduler"), prop);
    }

    virtual Json render() const override {
        scene_->require_renderable();
        film_->clear();
        const auto size = film_->size();

        // Execute parallel process
        const auto processed = sched_->run([&](long long, long long, int threadid) {
            // Per-thread random number generator
            thread_local Rng rng(seed_ ? *seed_ + threadid : math::rng_seed());

            // ------------------------------------------------------------------------------------

            // Sample primary ray
            const auto s_primary = path::sample_primary_ray(rng, scene_, TransDir::LE);

            // Intersection to next surface
            const auto hit_primary = scene_->intersect(s_primary->ray());
            if (!hit_primary) {
                return;
            }

            // ------------------------------------------------------------------------------------

            // Path throughput
            Vec3 throughput = s_primary->weight;

            // Current component
            const auto s_comp_primary_hit = path::sample_component(rng, scene_, *hit_primary);
            throughput *= s_comp_primary_hit.weight;

            // Current scene interaction and incident ray direction
            SceneInteraction sp = *hit_primary;
            Vec3 wi = -s_primary->wo;
            int comp = s_comp_primary_hit.comp;

            // ------------------------------------------------------------------------------------

            // Perform random walk
            for (int num_verts = 2; num_verts < max_verts_; num_verts++) {
                // Sample a NEE edge
                [&]{
                    // Sample a camera
                    const auto sE = path::sample_direct_camera(rng, scene_, sp);
                    if (!sE) {
                        return;
                    }
                    if (!scene_->visible(sp, sE->sp)) {
                        return;
                    }

                    // Compute raster position
                    const auto rp = path::raster_position(scene_, sE->wo);
                    if (!rp) {
                        return;
                    }

                    // Evaluate and accumulate contribution
                    const auto wo = -sE->wo;
                    const auto fs = path::eval_contrb_direction(scene_, sp, wi, wo, comp, TransDir::LE, true);
                    const auto C = throughput * fs * sE->weight;
                    film_->splat(*rp, C);
                }();

                // --------------------------------------------------------------------------------

                // Sample a direction based on current scene interaction
                const auto s = path::sample_direction(rng, scene_, sp, wi, comp, TransDir::LE);
                if (!s) {
                    break;
                }

                // --------------------------------------------------------------------------------

                // Intersection to next surface
                const auto hit = scene_->intersect({ sp.geom.p, s->wo });
                if (!hit) {
                    break;
                }

                // --------------------------------------------------------------------------------

                // Update throughput
                throughput *= s->weight;

                // --------------------------------------------------------------------------------

                // Russian roulette
                if (num_verts > 5) {
                    const auto q = glm::max(.2_f, 1_f - glm::compMax(throughput));
                    if (rng.u() < q) {
                        break;
                    }
                    throughput /= 1_f - q;
                }

                // --------------------------------------------------------------------------------

                // Sample component
                const auto s_comp = path::sample_component(rng, scene_, *hit);
                throughput *= s_comp.weight;

                // --------------------------------------------------------------------------------

                // Update
                wi = -s->wo;
                sp = *hit;
                comp = s_comp.comp;
            }
        });

        // Rescale film
        film_->rescale(Float(size.w * size.h) / processed);

        return { {"processed", processed} };
    }
};

LM_COMP_REG_IMPL(Renderer_LT_NEE, "renderer::lt");

LM_NAMESPACE_END(LM_NAMESPACE)
