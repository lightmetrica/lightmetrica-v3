/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/user.h>
#include <lm/renderer.h>
#include <lm/scene.h>
#include <lm/film.h>
#include <lm/json.h>
#include <lm/serial.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Renderer_Blank final : public Renderer {
private:
    Vec3 color_;
    Film* film_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(color_, film_);
    }

    virtual void foreachUnderlying(const ComponentVisitor& visit) override {
        comp::visit(visit, film_);
    }

    virtual void updateWeakRefs() override {
        comp::updateWeakRef(film_);
    }

public:
    virtual bool construct(const Json& prop) override {
        color_ = prop["color"];
        film_ = getAsset<Film>(prop, "output");
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
