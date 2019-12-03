/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/core.h>
#include <lm/renderer.h>
#include <lm/film.h>
#include <lm/volume.h>
#include <lm/scene.h>
#include <lm/phase.h>
#include <lm/scheduler.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// Renderer based on openvdb_render.cc in OpenVDB
class Renderer_OpenVDBRenderExample final : public Renderer {
private:
    Film* film_;
    const Volume* volume_;
    Float march_step_;
    Float march_step_shadow_;
    Vec3 light_dir_;
    Vec3 Le_;
    Vec3 muA_;       // Maximum absorption coefficient.
    Vec3 muS_;       // Maximum scattering coefficient.
    Vec3 muT_;       // Maximum extinction coefficient.
    Float cutoff_;
    Component::Ptr<scheduler::Scheduler> sched_;

public:
    virtual void construct(const Json& prop) override {
        film_ = json::comp_ref<Film>(prop, "output");
        volume_ = json::comp_ref<Volume>(prop, "volume");
        march_step_ = json::value<Float>(prop, "march_step", .5_f);
        march_step_shadow_ = json::value<Float>(prop, "march_step_shadow", 1_f);
        light_dir_ = glm::normalize(json::value<Vec3>(prop, "light_dir", Vec3(1_f)));
        Le_ = json::value<Vec3>(prop, "Le", Vec3(1_f));
        muA_ = json::value<Vec3>(prop, "muA", Vec3(.1_f));
        muS_ = json::value<Vec3>(prop, "muS", Vec3(1.5_f));
        muT_ = muA_ + muS_;
        cutoff_ = json::value<Float>(prop, "cutoff", 0.005_f);
        sched_ = comp::create<scheduler::Scheduler>(
            "scheduler::spp::sample", make_loc("scheduler"), {
                {"spp", 1},
                {"output", prop["output"]}
            });
    }
    
    // Assume volume stores density of the extinction coefficient and
    // densityScale_ is multipled to the evaluated density value.
    virtual void render(const Scene* scene) const override {
        film_->clear();
        const auto size = film_->size();

        // Compute colors for each pixel
        sched_->run([&](long long pixelIndex, long long, int) {
            const int x = int(pixelIndex % size.w);
            const int y = int(pixelIndex / size.w);

            // Generate primary ray
            const auto ray = scene->primary_ray({(x+.5_f)/size.w, (y+.5_f)/size.h}, film_->aspect_ratio());

            // Ray marching
            Vec3 L(0_f);
            Vec3 Tr(1_f);
            volume_->march(ray, Eps, Inf, march_step_, [&](Float t) {
                // Compute transmittance
                const auto p = ray.o + ray.d * t;
                const auto density = volume_->eval_scalar(p);
                const auto muT = muT_ * density;
                const auto T = glm::exp(-muT * march_step_);

                // Estimate transmittance along with the shadow ray
                // Assume there's no occlusions in the scene
                Ray shadow_ray{ p, light_dir_ };
                Vec3 Tr_shadow(1_f);
                volume_->march(shadow_ray, Eps, Inf, march_step_shadow_, [&](Float t_shadow) {
                    const auto p_shadow = shadow_ray.o + shadow_ray.d * t_shadow;
                    const auto density_shadow = volume_->eval_scalar(p_shadow);
                    const auto muT_shadow = muT_ * density_shadow;
                    const auto T_shadow = glm::exp(-muT_shadow * march_step_shadow_);
                    Tr_shadow *= T_shadow;
                    if (glm::length2(Tr_shadow) < cutoff_) {
                        return false;
                    }
                    return true;
                });
                
                // Accumulate contribution
                const auto C = muS_ / muT_ * Le_ * Tr * Tr_shadow * (1_f - T);
                L += C;
                Tr *= T;
                if (glm::length2(Tr) < cutoff_) {
                    return false;
                }

                return true;
            });

            // Record to the film
            film_->set_pixel(x, y, L);
        });
    }
};

LM_COMP_REG_IMPL(Renderer_OpenVDBRenderExample, "renderer::openvdb_render_example");

LM_NAMESPACE_END(LM_NAMESPACE)
