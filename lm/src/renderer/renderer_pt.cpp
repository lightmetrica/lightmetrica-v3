/*
    lightmetrica - copyright (c) 2018 hisanari otsu
    distributed under mit license. see license file for details.
*/

#include <pch.h>
#include <lm/renderer.h>
#include <lm/scene.h>
#include <lm/film.h>
#include <lm/parallel.h>
#include <lm/json.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Renderer_PT final : public Renderer {
private:
    Film* film_;
    long long spp_;
    int maxLength_;
    int rngSeed_ = 42;

public:
    virtual bool construct(const Json& prop) override {
        film_ = parent()->underlying<Film>(
            fmt::format("assets.{}", prop["output"].get<std::string>()));
        if (!film_) {
            return false;
        }
        spp_ = prop["spp"];
        maxLength_ = prop["maxLength"];
        return true;
    }

    virtual void render(const Scene& scene) const override {
        const auto [w, h] = film_->size();
        parallel::foreach(w*h, [&](long long index, int threadId) {
            // Per-thread random number generator
            thread_local Rng rng(rngSeed_ + threadId);

            // Raster position
            const int x = int(index % w);
            const int y = int(index / w);
            const Vec2 rp((x+.5_f)/w, (y+.5_f)/h);

            // Estimate pixel contribution
            Vec3 L;
            for (long long i = 0; i < spp_; i++) {
                Vec3 throughput(1_f);
                Scene::RaySampleCond cond(Scene::PixelPosition{x, y});
                for (int length = 0; length < maxLength_; length++) {
                    // Sample a ray
                    const auto sampled = scene.sampleRay(rng, cond);
                    if (!sampled) {
                        break;
                    }
                    const auto [ray, comp, weight] = *sampled;
                    
                    // Update throughput
                    throughput = throughput * weight;

                    // Intersection with next surface
                    const auto hit = scene.intersect(ray);
                    if (!hit) {
                        break;
                    }

                    // Accumulate contribution from light
                    if (scene.isLigth(*hit)) {
                        
                    }
                }
            }

            // Set color of the pixel
            film_->setPixel(x, y, L);
        });
    }
};

LM_COMP_REG_IMPL(Renderer_PT, "renderer::pt");

LM_NAMESPACE_END(LM_NAMESPACE)
