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

            // Pixel positions
            const int x = int(index % w);
            const int y = int(index / w);

            // Estimate pixel contribution
            Vec3 L;
            for (long long i = 0; i < spp_; i++) {
                // Path throughput
                Vec3 throughput(1_f);

                // Initial sampleRay function
                auto sampleRay = [&]() {
                    Float dx = 1_f/w, dy = 1_f/h;
                    return scene.samplePrimaryRay(rng, {dx*x, dy*y, dx, dy});
                };

                // Perform random walk
                for (int length = 0; length < maxLength_; length++) {
                    // Sample a ray
                    const auto s = sampleRay();
                    if (!s) {
                        break;
                    }

                    // Update throughput
                    throughput = throughput * s->weight;

                    // Intersection to next surface
                    const auto hit = scene.intersect(s->ray());
                    if (!hit) {
                        break;
                    }

                    // Accumulate contribution from light
                    if (scene.isLight(*hit) && !scene.isSpecular(s->sp)) {
                        L += throughput * scene.evalContrbEndpoint(*hit, -s->wo);
                    }

                    // Russian roulette
                    if (length > 3) {
                        const auto q = glm::max(.2_f, 1_f - glm::compMax(throughput));
                        throughput = throughput / (1_f - q);
                        if (rng.u() < q) {
                            return false;
                        }
                    }

                    // Update
                    sampleRay = [wi = -s->wo, sp = *hit]() {
                        return scene.sampleRay(rng, sp, wi);
                    };
                }
            }

            // Set color of the pixel
            film_->setPixel(x, y, L);
        });
    }
};

LM_COMP_REG_IMPL(Renderer_PT, "renderer::pt");

LM_NAMESPACE_END(LM_NAMESPACE)
