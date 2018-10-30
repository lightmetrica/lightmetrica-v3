/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/renderer.h>
#include <lm/scene.h>
#include <lm/film.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Renderer_Blank final : public Renderer {
private:
    vec3 color_;
    Film* film_;

public:
    virtual bool construct(const json& prop, Component* parent) override {
        color_ = lm::castFromJson<vec3>(prop["color"]);
        film_= parent->underlying<Film>(
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
                film_->setPixel(x, y, color_);
            }
        }
    }
};

LM_COMP_REG_IMPL(Renderer_Blank, "renderer::blank");

LM_NAMESPACE_END(LM_NAMESPACE)
