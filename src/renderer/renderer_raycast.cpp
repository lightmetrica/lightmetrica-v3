/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/renderer.h>
#include <lm/scene.h>
#include <lm/film.h>
#include <lm/parallel.h>
#include <lm/scheduler.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Renderer_Raycast final : public Renderer {
private:
    Vec3 bgColor_;
    bool useConstantColor_;
    bool visualizeNormal_;
    std::optional<Vec3> color_;
    Film* film_;
    Component::Ptr<scheduler::Scheduler> sched_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(bgColor_, useConstantColor_, visualizeNormal_, film_, sched_);
    }

    virtual void foreachUnderlying(const ComponentVisitor& visit) override {
        comp::visit(visit, film_);
        comp::visit(visit, sched_);
    }

    virtual Component* underlying(const std::string& name) const override {
        if (name == "scheduler") {
            return sched_.get();
        }
        return nullptr;
    }

public:
    virtual void construct(const Json& prop) override {
        bgColor_ = json::value(prop, "bg_color", Vec3(0_f));
        useConstantColor_ = json::value(prop, "use_constant_color", false);
        visualizeNormal_ = json::value(prop, "visualize_normal", false);
        color_ = json::valueOrNone<Vec3>(prop, "color");
        film_ = json::compRef<Film>(prop, "output");
        sched_ = comp::create<scheduler::Scheduler>(
            "scheduler::spp::sample", makeLoc("scheduler"), {
                {"spp", 1},
                {"output", prop["output"]}
            });
    }

    virtual void render(const Scene* scene) const override {
        film_->clear();
        const auto size = film_->size();
        sched_->run([&](long long index, long long, int) {
            const int x = int(index % size.w);
            const int y = int(index / size.w);
            const auto ray = scene->primaryRay({(x+.5_f)/size.w, (y+.5_f)/size.h}, film_->aspectRatio());
            const auto sp = scene->intersect(ray);
            if (!sp) {
                film_->setPixel(x, y, bgColor_);
                return;
            }
            if (visualizeNormal_) {
                film_->setPixel(x, y, glm::abs(sp->geom.n));
            }
            else {
                const auto R = color_ ? *color_ : scene->reflectance(*sp, -1);
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
