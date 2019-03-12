/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/camera.h>
#include <lm/film.h>
#include <lm/json.h>
#include <lm/user.h>
#include <lm/serial.h>
#include <lm/surface.h>

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
    Vec3 center_;     // Lookat position
    Vec3 up_;         // Up vector

    Vec3 u_, v_, w_;  // Basis for camera coordinates
    Float vfov_;      // Vertical field of view
    Float tf_;        // Half of the screen height at 1 unit forward from the position

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(film_, position_, center_, up_, u_, v_, w_, vfov_, tf_);
    }

    virtual void foreachUnderlying(const ComponentVisitor& visit) override {
        comp::visit(visit, film_);
    }

public:
    virtual Json underlyingValue(const std::string&) const override {
        return {
            {"eye", position_},
            {"center", center_},
            {"up", up_},
            {"vfov", vfov_}
        };
    }

    virtual bool construct(const Json& prop) override {
        film_ = getAsset<Film>(prop["film"]);       // Film
        if (!film_) {
            return false;
        }
        position_ = prop["position"];               // Camera position
        center_ = prop["center"];                   // Look-at position
        up_ = prop["up"];                           // Up vector
        vfov_ = prop["vfov"];                       // Vertical FoV
        tf_ = tan(vfov_ * Pi / 180_f * .5_f);       // Precompute half of screen height
        w_ = glm::normalize(position_ - center_);   // Compute basis
        u_ = glm::normalize(glm::cross(up_, w_));
        v_ = cross(w_, u_);
        return true;
    }

    virtual bool isSpecular(const PointGeometry&) const override {
        return false;
    }

    virtual Ray primaryRay(Vec2 rp) const override {
        rp = 2_f*rp-1_f;
        const auto d = glm::normalize(Vec3(film_->aspectRatio()*tf_*rp.x, tf_*rp.y, -1_f));
        return { position_, u_*d.x+v_*d.y+w_*d.z };
    }

    virtual std::optional<CameraRaySample> samplePrimaryRay(Rng& rng, Vec4 window) const override {
        const auto [x, y, w, h] = window.data.data;
        return CameraRaySample{
            PointGeometry::makeDegenerated(position_),
            primaryRay({x+w*rng.u(), y+h*rng.u()}).d,
            Vec3(1_f)
        };
    }

    virtual Vec3 eval(const PointGeometry& geom, Vec3 wo) const override {
        LM_UNUSED(geom, wo);
        LM_TBA_RUNTIME();
    }
};

LM_COMP_REG_IMPL(Camera_Pinhole, "camera::pinhole");

LM_NAMESPACE_END(LM_NAMESPACE)
