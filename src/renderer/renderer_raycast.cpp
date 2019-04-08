/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/user.h>
#include <lm/renderer.h>
#include <lm/scene.h>
#include <lm/film.h>
#include <lm/parallel.h>
#include <lm/json.h>
#include <lm/logger.h>
#include <lm/serial.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Renderer_Raycast final : public Renderer {
private:
    Vec3 bgColor_;
    bool useConstantColor_;
    bool visualizeNormal_;
    Film* film_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(bgColor_, useConstantColor_, visualizeNormal_, film_);
    }

    virtual void foreachUnderlying(const ComponentVisitor& visit) override {
        comp::visit(visit, film_);
    }

public:
    virtual bool construct(const Json& prop) override {
        bgColor_ = json::value(prop, "bg_color", Vec3(0_f));
        useConstantColor_ = json::value(prop, "use_constant_color", false);
        visualizeNormal_ = json::value(prop, "visualize_normal", false);
        film_ = comp::get<Film>(json::value<std::string>(prop, "output"));
        if (!film_) {
            return false;
        }
        return true;
    }

    virtual void render(const Scene* scene) const override {
        const auto [w, h] = film_->size();
        const long long samples = w * h;
        parallel::foreach(samples, [&](long long index, int threadId) {
            LM_UNUSED(threadId);
            const int x = int(index % w);
            const int y = int(index / w);
            const auto ray = scene->primaryRay({(x+.5_f)/w, (y+.5_f)/h}, film_->aspectRatio());
            const auto sp = scene->intersect(ray);
            if (!sp) {
                film_->setPixel(x, y, bgColor_);
                return;
            }
            if (visualizeNormal_) {
                film_->setPixel(x, y, glm::abs(sp->geom.n));
            }
            else {
                const auto R = scene->reflectance(*sp);
                auto C = R ? *R : Vec3();
                if (!useConstantColor_) {
                    C *= .2_f + .8_f*glm::abs(glm::dot(sp->geom.n, -ray.d));
                }
                film_->setPixel(x, y, C);
            }
        });
    }
};

LM_COMP_REG_IMPL(Renderer_Raycast, "renderer::raycast");

LM_NAMESPACE_END(LM_NAMESPACE)
