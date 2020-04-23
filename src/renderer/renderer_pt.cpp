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
#include <lm/timer.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Renderer_PT : public Renderer {
private:
    enum class SamplingMode {
        Naive,
        NEE,
        MIS,
    };

    enum class PrimaryRaySampleMode {
        Pixel,
        Image,
    };

private:
    Scene* scene_;                                      // Reference to scene asset
    Film* film_;                                        // Reference to film asset for output
    int max_verts_;                                     // Maximum number of path vertices
    std::optional<unsigned int> seed_;                  // Random seed
    SamplingMode sampling_mode_;                        // Sampling mode
    PrimaryRaySampleMode primary_ray_sampling_mode_;    // Sampling mode of the primary ray
    Component::Ptr<scheduler::Scheduler> sched_;        // Scheduler for parallel processing

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
        max_verts_ = json::value<int>(prop, "max_verts");
        seed_ = json::value_or_none<unsigned int>(prop, "seed");
        {
            const auto s = json::value<std::string>(prop, "sampling_mode", "mis");
            if (s == "naive")    sampling_mode_ = SamplingMode::Naive;
            else if (s == "nee") sampling_mode_ = SamplingMode::NEE;
            else if (s == "mis") sampling_mode_ = SamplingMode::MIS;
        }
        {

            const auto name = json::value<std::string>(prop, "scheduler");
            const auto s = json::value<std::string>(prop, "primary_ray_sampling_mode", "pixel");
            if (s == "pixel") {
                primary_ray_sampling_mode_ = PrimaryRaySampleMode::Pixel;
                sched_ = comp::create<scheduler::Scheduler>(
                    "scheduler::spp::" + name, make_loc("scheduler"), prop);
            }
            else if (s == "image") {
                primary_ray_sampling_mode_ = PrimaryRaySampleMode::Image;
                sched_ = comp::create<scheduler::Scheduler>(
                    "scheduler::spi::" + name, make_loc("scheduler"), prop);
            }
        }
    }

public:
    virtual Json render() const override {
        scene_->require_renderable();

        // Clear film
        film_->clear();
        const auto size = film_->size();
        timer::ScopedTimer st;

        // Execute parallel process
        const auto processed = sched_->run([&](long long pixel_index, long long, int threadid) {
            // Per-thread random number generator
            thread_local Rng rng(seed_ ? *seed_ + threadid : math::rng_seed());

            // ------------------------------------------------------------------------------------

            // Sample window
            const auto window = [&]() -> Vec4 {
                if (primary_ray_sampling_mode_ == PrimaryRaySampleMode::Pixel) {
                    const int x = int(pixel_index % size.w);
                    const int y = int(pixel_index / size.w);
                    const auto dx = 1_f / size.w;
                    const auto dy = 1_f / size.h;
                    return { dx * x, dy * y, dx, dy };
                }
                else {
                    return { 0_f, 0_f, 1_f, 1_f };
                }
            }();

            // ------------------------------------------------------------------------------------

            // Sample initial vertex
            const auto sE = path::sample_position(rng, scene_, TransDir::EL);
            const auto sE_comp = path::sample_component(rng, scene_, sE->sp, {});
            auto sp = sE->sp;
            int comp = sE_comp.comp;
            auto throughput = sE->weight * sE_comp.weight;

            // ------------------------------------------------------------------------------------

            // Perform random walk
            Vec3 wi{};
            Vec2 raster_pos{};
            for (int num_verts = 1; num_verts < max_verts_; num_verts++) {
                // Sample NEE edge

                // Flag indicating if the nee edge is samplable
                const bool samplable_by_nee = [&]() {
                    if (sampling_mode_ == SamplingMode::Naive) {
                        // Skip if sampling mode is naive
                        return false;
                    }
                    const auto is_specular = path::is_specular_component(scene_, sp, comp);
                    if (primary_ray_sampling_mode_ == PrimaryRaySampleMode::Pixel) {
                        // In pixel sampling mode, the nee edge is only samplable when nv>1
                        return num_verts > 1 && !is_specular;
                    }
                    else {
                        return !is_specular;
                    }
                }();

                if (samplable_by_nee) [&]{
                    // Sample a light
                    const auto sL = path::sample_direct(rng, scene_, sp, TransDir::LE);
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
                    const auto fs = path::eval_contrb_direction(scene_, sp, wi, wo, comp, TransDir::EL, true);
                    if (math::is_zero(fs)) {
                        return;
                    }

                    // Evaluate MIS weight
                    const auto mis_w = [&]() -> Float {
                        // Skip if sampling mode is NEE
                        if (sampling_mode_ == SamplingMode::NEE) {
                            return 1_f;
                        }

                        // When the light is not samplable by BSDF sampling, we will use only NEE.
                        // This includes, for instance, the light sampling for
                        // directional light, environment light, point light, etc.
                        const bool is_specular_L = path::is_specular_component(scene_, sL->sp, {});
                        const bool samplable_by_bsdf = !is_specular_L && !sL->sp.geom.degenerated;
                        if (!samplable_by_bsdf) {
                            return 1_f;
                        }

                        // MIS weight using balance heuristic
                        const auto p_light = path::pdf_direct(scene_, sp, sL->sp, sL->wo, true);
                        const auto p_bsdf = path::pdf_direction(scene_, sp, wi, wo, comp, true);
                        return math::balance_heuristic(p_light, p_bsdf);
                    }();

                    // Accumulate contribution
                    const auto C = throughput * fs * sL->weight * mis_w;
                    film_->splat(rp, C);
                }();

                // --------------------------------------------------------------------------------

                // Sample direction
                const auto s = [&]() -> std::optional<path::DirectionSample> {
                    if (num_verts == 1) {
                        const auto [x, y, w, h] = window.data.data;
                        const auto ud = Vec2(x+w*rng.u(), y+h*rng.u());
                        return path::sample_direction({ ud, rng.next<Vec2>() }, scene_, sp, wi, comp, TransDir::EL);
                    }
                    else {
                        return path::sample_direction(rng, scene_, sp, wi, comp, TransDir::EL);
                    }
                }();
                if (!s) {
                    break;
                }

                // --------------------------------------------------------------------------------

                // Compute and cache raster position
                if (num_verts == 1) {
                    raster_pos = *path::raster_position(scene_, s->wo);
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

                // Contribution from direct hit against a light

                // Flag indicating if the light can be samplable by direct hit
                const bool samplable_by_direct_hit = [&]() {
                    if (sampling_mode_ == SamplingMode::NEE) {
                        // Accumulate contribution from the direct hit only when a NEE edge is not samplable
                        return !samplable_by_nee;
                    }
                    else {
                        return true;
                    }
                }();

                if (samplable_by_direct_hit && scene_->is_light(*hit)) [&]{
                    // Compute contribution from the direct hit
                    const auto spL = hit->as_type(SceneInteraction::LightEndpoint);
                    const auto woL = -s->wo;
                    const auto fs = path::eval_contrb_direction(scene_, spL, {}, woL, comp, TransDir::LE, true);
                    const auto mis_w = [&]() -> Float {
                        // Skip if sampling mode is naive
                        if (sampling_mode_ == SamplingMode::Naive) {
                            return 1_f;
                        }

                        // The weight is one if the hit cannot be sampled by nee
                        if (!samplable_by_nee) {
                            return 1_f;
                        }

                        // MIS weight using balance heuristic
                        const auto pdf_bsdf = path::pdf_direction(scene_, sp, wi, s->wo, comp, true);
                        const auto pdf_light = path::pdf_direct(scene_, sp, spL, woL, true);
                        return math::balance_heuristic(pdf_bsdf, pdf_light);
                    }();

                    // Accumulate contribution
                    const auto C = throughput * fs * mis_w;
                    film_->splat(raster_pos, C);
                }();
                
                // --------------------------------------------------------------------------------

                // Termination on a hit with environment
                if (hit->geom.infinite) {
                    break;
                }

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
                const auto s_comp = path::sample_component(rng, scene_, *hit, -s->wo);
                throughput *= s_comp.weight;

                // --------------------------------------------------------------------------------

                // Update information
                wi = -s->wo;
                sp = *hit;
                comp = s_comp.comp;
            }
        });

        // ----------------------------------------------------------------------------------------
        
        // Rescale film
        if (primary_ray_sampling_mode_ == PrimaryRaySampleMode::Pixel) {
            film_->rescale(1_f / processed);
        }
        else {
            film_->rescale(Float(size.w * size.h) / processed);
        }

        return { {"processed", processed}, {"elapsed", st.now()} };
    }
};

LM_COMP_REG_IMPL(Renderer_PT, "renderer::pt");

LM_NAMESPACE_END(LM_NAMESPACE)
