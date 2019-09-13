/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/renderer.h>
#include <lm/json.h>
#include <lm/film.h>
#include <lm/volume.h>
#include <lm/parallel.h>
#include <lm/scene.h>
#include <lm/phase.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// Renderer based on openvdb_render.cc in OpenVDB
class Renderer_OpenVDBRenderExample final : public Renderer {
private:
    Film* film_;
    const Volume* volume_;
    Float marchStep_;
    Float marchStepShadow_;
    Vec3 lightDir_;
    Vec3 Le_;
    Vec3 muA_;       // Maximum absorption coefficient.
    Vec3 muS_;       // Maximum scattering coefficient.
    Vec3 muT_;       // Maximum extinction coefficient.
    Float cutoff_;

public:
    virtual bool construct(const Json& prop) override {
        film_ = json::compRef<Film>(prop, "output");
        volume_ = json::compRef<Volume>(prop, "volume");
        marchStep_ = json::value<Float>(prop, "march_step", .5_f);
        marchStepShadow_ = json::value<Float>(prop, "march_step_shadow", 1_f);
        lightDir_ = glm::normalize(json::value<Vec3>(prop, "light_dir", Vec3(1_f)));
        Le_ = json::value<Vec3>(prop, "Le", Vec3(1_f));
        muA_ = json::value<Vec3>(prop, "muA", Vec3(.1_f));
        muS_ = json::value<Vec3>(prop, "muS", Vec3(1.5_f));
        muT_ = muA_ + muS_;
        cutoff_ = json::value<Float>(prop, "cutoff", 0.005_f);
        return true;
    }
    
    // Assume volume stores density of the extinction coefficient and
    // densityScale_ is multipled to the evaluated density value.
    virtual void render(const Scene* scene) const override {
        film_->clear();
        const auto [w, h] = film_->size();

        // Compute colors for each pixel
        parallel::foreach(w*h, [&](long long index, int) {
            const int x = int(index % w);
            const int y = int(index / w);

            // Generate primary ray
            const auto ray = scene->primaryRay({(x+.5_f)/w, (y+.5_f)/h}, film_->aspectRatio());

            // Ray marching
            Vec3 L(0_f);
            Vec3 Tr(1_f);
            volume_->march(ray, Eps, Inf, marchStep_, [&](Vec3 p) {
                // Compute transmittance
                const auto density = volume_->evalScalar(p);
                const auto muT = muT_ * density;
                const auto T = glm::exp(-muT * marchStep_);

                // Estimate transmittance along with the shadow ray
                // Assume there's no occlusions in the scene
                Ray shadowRay{ p, lightDir_ };
                Vec3 Tr_shadow(1_f);
                volume_->march(shadowRay, Eps, Inf, marchStepShadow_, [&](Vec3 p_shadow) {
                    const auto density_shadow = volume_->evalScalar(p_shadow);
                    const auto muT_shadow = muT_ * density_shadow;
                    const auto T_shadow = glm::exp(-muT_shadow * marchStepShadow_);
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
            film_->setPixel(x, y, L);
        });
    }
};

LM_COMP_REG_IMPL(Renderer_OpenVDBRenderExample, "renderer::openvdb_render_example");

LM_NAMESPACE_END(LM_NAMESPACE)
