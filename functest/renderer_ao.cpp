/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Renderer_AO final : public Renderer {
private:
    Film* film_;
    long long spp_;
    int rngSeed_ = 42;

public:
    virtual void construct(const Json& prop) override {
        film_ = json::compRef<Film>(prop, "output");
        spp_ = json::value<long long>(prop, "spp");
    }

    virtual void render(const Scene* scene) const override {
        const auto [w, h] = film_->size();
        parallel::foreach(w*h, [&](long long index, int threadId) -> void {
            thread_local Rng rng(rngSeed_ + threadId);
            const int x = int(index % w);
            const int y = int(index / w);
            const auto ray = scene->primaryRay({(x+.5_f)/w, (y+.5_f)/h}, film_->aspectRatio());
            const auto hit = scene->intersect(ray);
            if (!hit) {
                return;
            }
            auto V = 0_f;
            for (long long i = 0; i < spp_; i++) {
                const auto [n, u, v] = hit->geom.orthonormalBasis(-ray.d);
                const auto d = math::sampleCosineWeighted(rng);
                V += scene->intersect({hit->geom.p, u*d.x+v*d.y+n*d.z}, Eps, .2_f) ? 0_f : 1_f;
            }
            V /= spp_;
            film_->setPixel(x, y, Vec3(V));
        });
    }
};

LM_COMP_REG_IMPL(Renderer_AO, "renderer::ao");

LM_NAMESPACE_END(LM_NAMESPACE)
