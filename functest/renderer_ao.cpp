/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Renderer_AO final : public Renderer {
private:
    Scene* scene_;
    Film* film_;
    long long spp_;
    int rng_seed_ = 42;

public:
    virtual void construct(const Json& prop) override {
        scene_ = json::comp_ref<Scene>(prop, "scene");
        film_ = json::comp_ref<Film>(prop, "output");
        spp_ = json::value<long long>(prop, "spp");
    }

    virtual Json render() const override {
        const auto size = film_->size();
        parallel::foreach(size.w*size.h, [&](long long index, int threadId) -> void {
            thread_local Rng rng(rng_seed_ + threadId);
            const int x = int(index % size.w);
            const int y = int(index / size.w);
            const auto ray = path::primary_ray(scene_, {(x+.5_f)/size.w, (y+.5_f)/size.h});
            const auto hit = scene_->intersect(ray);
            if (!hit) {
                return;
            }
            auto V = 0_f;
            for (long long i = 0; i < spp_; i++) {
                const auto [n, u, v] = hit->geom.orthonormal_basis_twosided(-ray.d);
                const auto d = math::sample_cosine_weighted(rng.next<Vec2>());
                V += scene_->intersect({hit->geom.p, u*d.x+v*d.y+n*d.z}, Eps, .2_f) ? 0_f : 1_f;
            }
            V /= spp_;
            film_->set_pixel(x, y, Vec3(V));
        });
        return {};
    }
};

LM_COMP_REG_IMPL(Renderer_AO, "renderer::ao");

LM_NAMESPACE_END(LM_NAMESPACE)
