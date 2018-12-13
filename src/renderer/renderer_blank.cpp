/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/user.h>
#include <lm/renderer.h>
#include <lm/scene.h>
#include <lm/film.h>
#include <lm/json.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Renderer_Blank final : public Renderer {
private:
    Vec3 color_;
    Film* film_;

public:
    virtual bool construct(const Json& prop) override {
        color_ = prop["color"];
        film_ = comp::cast<lm::Film>(lm::getAsset(prop["output"].get<std::string>()));
        if (!film_) {
            return false;
        }
        return true;
    }

    virtual void render(const Scene* scene) const override {
        LM_UNUSED(scene);
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
