/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/renderer.h>
#include <lm/scene.h>
#include <lm/film.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Renderer_Raycast final : public Renderer {
private:
    Vec3 color_;
    Film* film_;

public:
    virtual bool construct(const Json& prop) override {
        color_ = castFromJson<Vec3>(prop["color"]);
        film_ = parent()->underlying<Film>(
            fmt::format("assets.{}", prop["output"].get<std::string>()));
        if (!film_) {
            return false;
        }
        return true;
    }

    virtual void render(const Scene& scene) const override {
        const auto [w, h] = film_->size();
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                const auto ray = scene.primaryRay({(x+.5_f)/w, (y+.5_f)/h});
                const auto sp = scene.intersect(ray);
                if (!sp) {
                    film_->setPixel(x, y, color_);
                    continue;
                }
                film_->setPixel(x, y, Vec3(glm::abs(glm::dot(sp->n, -ray.d))));
                //film_->setPixel(x, y, Vec3(glm::abs(sp->n)));
            }
        }
    }
};

LM_COMP_REG_IMPL(Renderer_Raycast, "renderer::raycast");

LM_NAMESPACE_END(LM_NAMESPACE)
