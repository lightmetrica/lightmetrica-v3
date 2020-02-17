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

class Renderer_PT : public Renderer {
private:
    enum class SamplingMode {
        Naive,
        NEE,
        MIS,
    };

private:
    Scene* scene_;                                  // Reference to scene asset
    Film* film_;                                    // Reference to film asset for output
    int max_verts_;                                 // Maximum number of path vertices
    std::optional<unsigned int> seed_;              // Random seed
    SamplingMode sampling_mode_;                    // Sampling mode
    Component::Ptr<scheduler::Scheduler> sched_;    // Scheduler for parallel processing

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(scene_, film_, max_verts_, sampling_mode_, sched_);
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
        {
            const auto s = json::value<std::string>(prop, "sampling_mode", "mis");
            if (s == "naive")    sampling_mode_ = SamplingMode::Naive;
            else if (s == "nee") sampling_mode_ = SamplingMode::NEE;
            else if (s == "mis") sampling_mode_ = SamplingMode::MIS;
        }
        {
            const auto sched_name = json::value<std::string>(prop, "scheduler");
            sched_ = comp::create<scheduler::Scheduler>(
                "scheduler::spi::" + sched_name, make_loc("scheduler"), prop);
        }
    }

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

            // ------------------------------------------------------------------------------------

            // Sample initial vertex
            const auto sE = path::sample_position(rng, scene_, TransDir::EL);
            auto sp = sE->sp;
            auto throughput = sE->weight;

            // ------------------------------------------------------------------------------------

            // Perform random walk
            Vec3 wi{};
            Vec2 raster_pos{};
            for (int num_verts = 1; num_verts < max_verts_; num_verts++) {
                // Sample direction
                const auto s = path::sample_direction(rng, scene_, sp, wi, TransDir::EL);
                if (!s) {
                    break;
                }

                // --------------------------------------------------------------------------------

                // Compute and cache raster position
                if (num_verts == 1) {
                    raster_pos = *path::raster_position(scene_, s->wo);
                }

                // --------------------------------------------------------------------------------

                // Sample NEE edge
                [&]{
                    // Skip if sampling mode is naive
                    if (sampling_mode_ == SamplingMode::Naive) {
                        return;
                    }

                    // Sample a light
                    const auto sL = path::sample_direct_light(rng, scene_, sp);
                    if (!sL) {
                        return;
                    }
                    if (!scene_->visible(sp, sL->sp)) {
                        return;
                    }

                    // Recompute raster position for the primary edge
                    Vec2 rp = raster_pos;
                    if (num_verts == 1) {
                        const auto rp_ = path::raster_position(scene_, -sL->wo);
                        if (!rp_) { return; }
                        rp = *rp_;
                    }

                    // Evaluate BSDF
                    const auto wo = -sL->wo;
                    const auto fs = path::eval_contrb_direction(scene_, sp, wi, wo, TransDir::EL, true);

                    // Evaluate MIS weight
                    const auto mis_w = [&]() -> Float {
                        // Skip if sampling mode is NEE
                        if (sampling_mode_ == SamplingMode::NEE) {
                            return 1_f;
                        }

                        // When the light is not samplable by BSDF sampling, we will use only NEE.
                        // This includes, for instance, the light sampling for
                        // directional light, environment light, point light, etc.
                        const bool is_samplable_by_bsdf_sampling = !s->specular && !sL->sp.geom.degenerated;
                        if (!is_samplable_by_bsdf_sampling) {
                            return 1_f;
                        }

                        // MIS weight using balance heuristic
                        const auto p_light = path::pdf_direct(scene_, sp, sL->sp, sL->wo);
                        const auto p_bsdf = path::pdf_direction(scene_, sp, wi, wo, true);
                        return math::balance_heuristic(p_light, p_bsdf);
                    }();

                    // Accumulate contribution
                    const auto C = throughput * fs * sL->weight * mis_w;
                    film_->splat(rp, C);
                }();

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

                // Contribution from direct hit against a light
                [&]{
                    if (sampling_mode_ == SamplingMode::NEE) {
                        // Accumulate contribution from light only when a NEE edge is not samplable.
                        // This happens when the direction is sampled from delta distribution.
                        // Note that this include the case where the surface has multi-component material
                        // containing delta component and the direction is sampled from the component.
                        const auto samplable_by_nee = !s->specular;
                        if (samplable_by_nee) {
                            return;
                        }
                    }

                    // Check hit with a light
                    if (!scene_->is_light(*hit)) {
                        return;
                    }

                    // Compute contribution from the direct hit
                    const auto spL = hit->as_type(SceneInteraction::LightEndpoint);
                    const auto woL = -s->wo;
                    const auto fs = path::eval_contrb_direction(scene_, spL, {}, woL, TransDir::LE, {});
                    const auto mis_w = [&]() -> Float {
                        // Skip if sampling mode is naive
                        if (sampling_mode_ == SamplingMode::Naive) {
                            return 1_f;
                        }

                        // If the light is not samplable by NEE, we will use only BSDF sampling.
                        const auto is_samplable_by_nee = !s->specular;
                        if (!is_samplable_by_nee) {
                            return 1_f;
                        }

                        // MIS weight using balance heuristic
                        const auto pdf_bsdf = path::pdf_direction(scene_, sp, wi, s->wo, true);
                        const auto pdf_light = path::pdf_direct(scene_, sp, spL, woL);
                        return math::balance_heuristic(pdf_bsdf, pdf_light);
                    }();

                    // Accumulate contribution
                    const auto C = throughput * fs * mis_w;
                    film_->splat(raster_pos, C);
                }();
                
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

                // Update information
                wi = -s->wo;
                sp = *hit;
            }
        });

        // ----------------------------------------------------------------------------------------
        
        // Rescale film
        film_->rescale(Float(size.w * size.h) / processed);

        return { {"processed", processed} };
    }
};

LM_COMP_REG_IMPL(Renderer_PT, "renderer::pt");

LM_NAMESPACE_END(LM_NAMESPACE)
