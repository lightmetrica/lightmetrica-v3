/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/camera.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Camera_Pinhole final : public Camera {
private:
    Vec3 position_;    // Sensor position
    Vec3 u_, v_, w_;   // Basis for camera coordinates
    Float tf_;         // Half of the screen height at 1 unit forward from the position
    Float aspect_;     // Aspect ratio

public:
    virtual bool construct(const Json& prop) override {
        // Camera position
        position_ = castFromJson<Vec3>(prop["position"]);
        // Look-at position
        const auto center = castFromJson<Vec3>(prop["center"]);
        // Up vector
        const auto up = castFromJson<Vec3>(prop["up"]);
        // Vertical FoV
        const Float fv = prop["vfov"];
        // Aspect ratio
        aspect_ = prop["aspect"];
        // Precompute half of screen height
        tf_ = tan(fv * Pi / 180_f * .5_f);
        // Compute basis
        w_ = glm::normalize(position_ - center);
        u_ = glm::normalize(glm::cross(up, w_));
        v_ = cross(w_, u_);
        return true;
    }

    virtual Ray primaryRay(Vec2 rp) const override {
        rp = 2_f*rp-1_f;
        const auto d = -glm::normalize(Vec3(aspect_*tf_*rp.x, tf_*rp.y, 1_f));
        return { position_, u_*d.x+v_*d.y+w_*d.z };
    }
};

LM_COMP_REG_IMPL(Camera_Pinhole, "camera::pinhole");

LM_NAMESPACE_END(LM_NAMESPACE)
