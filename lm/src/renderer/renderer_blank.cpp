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
    // Background color
    vec3 color_;

public:
    virtual bool construct(const json& prop, Component* parent) override {
        color_ = lm::castFromJson<vec3>(prop["color"]);
    }

    virtual void render(Component* ctx) const override {
        auto* film = ctx->underlying<Film>("assets.film");
        const auto [w, h] = film->size();
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                film->setPixel(x, y, color_);
            }
        }
    }
};

LM_COMP_REG_IMPL(Renderer_Blank, "renderer::blank");

LM_NAMESPACE_END(LM_NAMESPACE)
