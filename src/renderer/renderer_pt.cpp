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

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

enum class PTMode {
    Naive,
    NEE,
    MIS,
};

enum class ImageSampleMode {
    Pixel,
    Image,
};

// ------------------------------------------------------------------------------------------------

class Renderer_PT : public Renderer {
private:
    Scene* scene_;
    Film* film_;
    int max_length_;
    std::optional<unsigned int> seed_;
    PTMode pt_mode_;
    ImageSampleMode image_sample_mode_;
    Component::Ptr<scheduler::Scheduler> sched_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(scene_, film_, max_length_, pt_mode_, sched_);
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
        seed_ = json::value_or_none<unsigned int>(prop, "seed");
        {
            const auto s = json::value<std::string>(prop, "mode", "mis");
            if (s == "naive") {
                pt_mode_ = PTMode::Naive;
            }
            else if (s == "nee") {
                pt_mode_ = PTMode::NEE;
            }
            else if (s == "mis") {
                pt_mode_ = PTMode::MIS;
            }
        }
        {
            const auto sched_name = json::value<std::string>(prop, "scheduler");
            const auto s = json::value<std::string>(prop, "image_sample_mode", "pixel");
            if (s == "pixel") {
                image_sample_mode_ = ImageSampleMode::Pixel;
                sched_ = comp::create<scheduler::Scheduler>(
                    "scheduler::spp::" + sched_name, make_loc("scheduler"), prop);
            }
            else if (s == "image") {
                image_sample_mode_ = ImageSampleMode::Image;
                sched_ = comp::create<scheduler::Scheduler>(
                    "scheduler::spi::" + sched_name, make_loc("scheduler"), prop);
            }
        }
    }

    virtual void render() const override {
        scene_->require_renderable();

        // Clear film
        film_->clear();
        const auto size = film_->size();

        // Execute parallel process
        const auto processed = sched_->run([&](long long pixel_index, long long, int threadid) {
            // Per-thread random number generator
            thread_local Rng rng(seed_ ? *seed_ + threadid : math::rng_seed());

            // ------------------------------------------------------------------------------------

            // Sample window
            const auto window = [&]() -> Vec4 {
                if (image_sample_mode_ == ImageSampleMode::Pixel) {
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

            // Path throughput
            Vec3 throughput(1_f);

            // Incident direction and current surface point
            Vec3 wi = {};
            auto sp = SceneInteraction::make_camera_terminator(window, film_->aspect_ratio());

            // Raster position
            Vec2 raster_pos{};

            // Perform random walk
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

                // --------------------------------------------------------------------------------

                // Sample a NEE edge
                const bool nee = [&]() {
                    // Ignore NEE edge with naive direct sampling mode
                    if (pt_mode_ == PTMode::Naive) {
                        return false;
                    }
                    // NEE edge can be samplable if current direction sampler
                    // (according to BSDF / phase) doesn't contain delta component.
                    if (image_sample_mode_ == ImageSampleMode::Pixel) {
                        // Primary ray is not samplable via NEE in the pixel space sample mode
                        return length > 0 && !scene_->is_specular(s->sp, s->comp);
                    }
                    else {
                        // Primary ray is samplable via NEE in the image space sample mode
                        return !scene_->is_specular(s->sp, s->comp);
                    }
                }();
                if (nee) [&] {
                    // Sample a light
                    const auto sL = scene_->sample_direct_light(rng, s->sp);
                    if (!sL) {
                        return;
                    }
                    if (!scene_->visible(s->sp, sL->sp)) {
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

                    // This light is not samplable by direct strategy
                    // if the light contain delta component or degenerated.
                    const bool directL = !scene_->is_specular(sL->sp, sL->comp) && !sL->sp.geom.degenerated;

                    // Evaluate and accumulate contribution
                    const auto wo = -sL->wo;
                    const auto fs = scene_->eval_contrb(s->sp, s->comp, wi, wo);
                    const auto pdf_sel = scene_->pdf_comp(s->sp, s->comp, wi);
                    const auto misw = [&]() -> Float {
                        if (pt_mode_ == PTMode::NEE) {
                            return 1_f;
                        }
                        if (!directL) {
                            return 1_f;
                        }
                        // Compute MIS weight only when wo can be sampled with both strategies.
                        return math::balance_heuristic(
                            scene_->pdf_direct_light(s->sp, sL->sp, sL->comp, sL->wo), 
                            scene_->pdf(s->sp, s->comp, wi, wo));
                    }();
                    const auto C = throughput / pdf_sel * fs * sL->weight * misw;
                    film_->splat(*rp, C);
                }();

                // --------------------------------------------------------------------------------

                // Intersection to next surface
                const auto hit = scene_->intersect(s->ray());
                if (!hit) {
                    break;
                }

                // --------------------------------------------------------------------------------

                // Update throughput
                throughput *= s->weight;

                // --------------------------------------------------------------------------------

                // Accumulate contribution from light
                const bool direct = [&]() -> bool {
                    // Direct strategy is samplable if the ray hit with light
                    if (pt_mode_ == PTMode::NEE) {
                        // In NEE mode, use direct strategy only when a NEE edge cannot be sampled.
                        return !nee && scene_->is_light(*hit);
                    }
                    else {
                        return scene_->is_light(*hit);
                    }
                }();
                if (direct) {
                    const auto woL = -s->wo;
                    const auto fs = scene_->eval_contrb_endpoint(*hit, woL);
                    const auto misw = [&]() -> Float {
                        if (pt_mode_ == PTMode::Naive) {
                            return 1_f;
                        }
                        if (!nee) {
                            return 1_f;
                        }
                        // The continuation edge can be sampled via both direct and NEE
                        return math::balance_heuristic(
                            scene_->pdf(s->sp, s->comp, wi, s->wo),
                            scene_->pdf_direct_light(s->sp, *hit, -1, woL));
                    }();
                    const auto C = throughput * fs * misw;
                    film_->splat(raster_pos, C);
                }

                // --------------------------------------------------------------------------------

                // Russian roulette
                if (length > 3) {
                    const auto q = glm::max(.2_f, 1_f - glm::compMax(throughput));
                    if (rng.u() < q) {
                        break;
                    }
                    throughput /= 1_f - q;
                }

                // --------------------------------------------------------------------------------

                // Update
                wi = -s->wo;
                sp = *hit;
            }
        });

        // ----------------------------------------------------------------------------------------
        
        // Rescale film
        if (image_sample_mode_ == ImageSampleMode::Pixel) {
            film_->rescale(1_f / processed);
        }
        else {
            film_->rescale(Float(size.w * size.h) / processed);
        }
    }
};

LM_COMP_REG_IMPL(Renderer_PT, "renderer::pt");

LM_NAMESPACE_END(LM_NAMESPACE)
