/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/renderer.h>
#include <lm/scene.h>
#include <lm/camera.h>
#include <lm/film.h>
#include <lm/parallel.h>
#include <lm/scheduler.h>
#include <lm/path.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Renderer_Raycast final : public Renderer {
private:
    Scene* scene_;
    Film* film_;
    Vec3 bg_color_;
    bool use_constant_color_;
    bool visualize_normal_;
    std::optional<Vec3> color_;
    Component::Ptr<scheduler::Scheduler> sched_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(scene_, film_, bg_color_, use_constant_color_, visualize_normal_, sched_);
    }

    virtual void foreach_underlying(const ComponentVisitor& visit) override {
        comp::visit(visit, scene_);
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
        scene_ = json::comp_ref<Scene>(prop, "scene");
        bg_color_ = json::value(prop, "bg_color", Vec3(0_f));
        use_constant_color_ = json::value(prop, "use_constant_color", false);
        visualize_normal_ = json::value(prop, "visualize_normal", false);
        color_ = json::value_or_none<Vec3>(prop, "color");
        film_ = json::comp_ref<Film>(prop, "output");
        scene_->camera()->set_aspect_ratio(film_->aspect());
        sched_ = comp::create<scheduler::Scheduler>(
            "scheduler::spp::sample", make_loc("scheduler"), {
                {"spp", 1},
                {"output", prop["output"]}
            });
    }

    virtual Json render() const override {
		scene_->require_primitive();
		scene_->require_accel();
		scene_->require_camera();

        film_->clear();
        const auto size = film_->size();
        sched_->run([&](long long index, long long, int) {
            const int x = int(index % size.w);
            const int y = int(index / size.w);
            const auto ray = path::primary_ray(scene_, {(x+.5_f)/size.w, (y+.5_f)/size.h});
            const auto sp = scene_->intersect(ray);
            if (!sp) {
                film_->set_pixel(x, y, bg_color_);
                return;
            }
            if (visualize_normal_) {
                film_->set_pixel(x, y, glm::abs(sp->geom.n));
            }
            else {
                const auto R = color_ ? *color_ : path::reflectance(scene_ , *sp);
                auto C = R ? *R : Vec3();
                if (!use_constant_color_) {
                    C *= .2_f + .8_f*glm::abs(glm::dot(sp->geom.n, -ray.d));
                }
                film_->set_pixel(x, y, C);
            }
        });

        return {};
    }
};

LM_COMP_REG_IMPL(Renderer_Raycast, "renderer::raycast");

LM_NAMESPACE_END(LM_NAMESPACE)
