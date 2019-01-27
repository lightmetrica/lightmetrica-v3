/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/camera.h>
#include <lm/film.h>
#include <lm/json.h>
#include <lm/scene.h>
#include <lm/user.h>
#include <lm/serial.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*
\rst
.. function:: camera::pinhole

   Pinhole camera.

   :param str film: Underlying film specified by asset name or locator.
   :param vec3 position: Camera position.
   :param vec3 center: Look-at position.
   :param vec3 up: Up vector.
   :param float vfov: Vertical field of view.
\endrst
*/
class Camera_Pinhole final : public Camera {
private:
    Film* film_;      // Underlying film
    Vec3 position_;   // Camera position
    Vec3 u_, v_, w_;  // Basis for camera coordinates
    Float tf_;        // Half of the screen height at 1 unit forward from the position
    Float aspect_;    // Aspect ratio

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(film_, position_, u_, v_, w_, tf_, aspect_);
    }

    virtual void foreachUnderlying(const ComponentVisitor& visit) override {
        comp::visit(visit, film_);
    }

public:
    virtual Component* underlying(const std::string& name) const {
        LM_UNUSED(name);
        return film_;
    }

    virtual bool construct(const Json& prop) override {
        film_ = getAsset<Film>(prop["film"]);       // Film
        if (!film_) {
            return false;
        }
        aspect_ = film_->aspectRatio();             // Aspect ratio
        position_ = prop["position"];               // Camera position
        const Vec3 center = prop["center"];         // Look-at position
        const Vec3 up = prop["up"];                 // Up vector
        const Float fv = prop["vfov"];              // Vertical FoV
        tf_ = tan(fv * Pi / 180_f * .5_f);          // Precompute half of screen height
        w_ = glm::normalize(position_ - center);    // Compute basis
        u_ = glm::normalize(glm::cross(up, w_));
        v_ = cross(w_, u_);
        return true;
    }

    virtual bool isSpecular(const SurfacePoint& sp) const override {
        LM_UNUSED(sp);
        return false;
    }

    virtual Ray primaryRay(Vec2 rp) const override {
        rp = 2_f*rp-1_f;
        const auto d = glm::normalize(Vec3(film_->aspectRatio()*tf_*rp.x, tf_*rp.y, -1_f));
        return { position_, u_*d.x+v_*d.y+w_*d.z };
    }

    virtual std::optional<RaySample> samplePrimaryRay(Rng& rng, Vec4 window) const override {
        const auto [x, y, w, h] = window.data.data;
        return RaySample(
            SurfacePoint(position_),
            primaryRay({x+w*rng.u(), y+h*rng.u()}).d,
            Vec3(1_f)
        );
    }

    virtual Vec3 eval(const SurfacePoint& sp, Vec3 wo) const override {
        LM_UNUSED(sp, wo);
        LM_TBA_RUNTIME();
    }
};

LM_COMP_REG_IMPL(Camera_Pinhole, "camera::pinhole");

LM_NAMESPACE_END(LM_NAMESPACE)
