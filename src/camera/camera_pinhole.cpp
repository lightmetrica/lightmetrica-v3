/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/camera.h>
#include <lm/film.h>

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

   This component implements pinhole camera where all the incoming lights pass through
   an small aperture and projected onto a film in the opposite side of the aperture.
   Unlike real pinhole camera, the apearture is modeled as a point,
   and the film can be placed in front of the pinhole.
   
   The configuration of the pinhole camera is described by a 3-tuple by
   ``position``, ``center``, and ``up`` vector.
   ``position`` represents a position of the pinhole,
   ``center`` for look-at position. This means the camera faces toward
   the direction to ``center`` from ``position``.
   ``up`` describes the upward direction of the camera.

   Field of view (FoV) describe the extent of the viewing angle of the camera.
   In this implementation, the configuration is given by ``vfov`` parameter.
   Note that we adopted vertical FoV. Be careful if you want to convert from
   other tools that might adopt horizontal FoV.
\endrst
*/
class Camera_Pinhole final : public Camera {
private:
    Vec3 position_;   // Camera position
    Vec3 center_;     // Lookat position
    Vec3 up_;         // Up vector

    Vec3 u_, v_, w_;  // Basis for camera coordinates
    Float vfov_;      // Vertical field of view
    Float tf_;        // Half of the screen height at 1 unit forward from the position

    Float aspect_;              // Aspect raio (height / width)
    Float preferred_aspect_;    // Preferred aspect ratio

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(position_, center_, up_, u_, v_, w_, vfov_, tf_);
    }

public:
    virtual Json underlying_value(const std::string&) const override {
        return {
            {"eye", position_},
            {"center", center_},
            {"up", up_},
            {"vfov", vfov_},
            {"preferred_aspect", preferred_aspect_}
        };
    }

    virtual void construct(const Json& prop) override {
        const auto it = prop.find("matrix");
        if (it != prop.end()) {
            Mat4 viewM = *it;
            position_ = Vec3(viewM[3]);
            const auto viewM3 = Mat3(viewM);
            u_ = -viewM3[0];
            v_ = viewM3[1];
            w_ = -viewM3[2];
        }
        else {
            position_ = json::value<Vec3>(prop, "position"); // Camera position
            center_ = json::value<Vec3>(prop, "center");     // Look-at position
            up_ = json::value<Vec3>(prop, "up");             // Up vector
            w_ = glm::normalize(position_ - center_);        // Compute basis
            u_ = glm::normalize(glm::cross(up_, w_));
            v_ = cross(w_, u_);
        }
        vfov_ = json::value<Float>(prop, "vfov");        // Vertical FoV
        tf_ = tan(vfov_ * Pi / 180_f * .5_f);            // Precompute half of screen height
        preferred_aspect_ = json::value<Float>(prop, "preferred_aspect", 1_f);
    }

private:
    // Compute Jacobian
    // TODO. Add derivation in documentataion
    Float J(Vec3 wo) const {
        const auto V = glm::transpose(Mat3(u_, v_, w_));
        const auto wo_eye = V * wo;
        const Float cos_theta = -wo_eye.z;
        const Float inv_cos_theta = 1_f / cos_theta;
        const Float A = tf_ * tf_ * aspect_ * 4_f;
        return inv_cos_theta * inv_cos_theta * inv_cos_theta / A;
    }

public:
    virtual void set_aspect_ratio(Float aspect) override {
        aspect_ = aspect;
    }

    // --------------------------------------------------------------------------------------------

    virtual Mat4 view_matrix() const override {
        return glm::lookAt(position_, position_ - w_, up_);
    }

    virtual Mat4 projection_matrix() const override {
        return glm::perspective(glm::radians(vfov_), aspect_, 0.01_f, 10000_f);
    }

    // --------------------------------------------------------------------------------------------

    virtual std::optional<Vec2> raster_position(Vec3 wo) const override {
        // Convert to camera space
        const auto to_eye = glm::transpose(Mat3(u_, v_, w_));
        const auto wo_eye = to_eye * wo;
        if (wo_eye.z >= 0) {
            // wo is directed to the opposition direction
            return {};
        }

        // Calculate raster position
        const auto rp = Vec2(
            -wo_eye.x/wo_eye.z/tf_/aspect_,
            -wo_eye.y/wo_eye.z/tf_)*.5_f + .5_f;
        if (rp.x < 0_f || rp.x > 1_f || rp.y < 0_f || rp.y > 1_f) {
            // wo is not in the view frustum
            return {};
        }
        
        return rp;
    }

    virtual Vec3 eval(Vec3 wo) const override {
        if (!raster_position(wo)) {
            return Vec3(0_f);
        }
        return Vec3(J(wo));
    }

    // --------------------------------------------------------------------------------------------

    virtual Ray primary_ray(Vec2 rp) const override {
        rp = 2_f*rp - 1_f;
        const auto d = glm::normalize(Vec3(aspect_*tf_*rp.x, tf_*rp.y, -1_f));
        return { position_, u_*d.x + v_ * d.y + w_ * d.z };
    }

    virtual std::optional<RaySample> sample_ray(const RaySampleU& u) const override {
        return RaySample{
            PointGeometry::make_degenerated(position_),
            primary_ray(u.ud).d,
            Vec3(1_f)
        };
    }

    virtual std::optional<DirectionSample> sample_direction(const DirectionSampleU& u) const override {
        return DirectionSample{
            primary_ray(u.ud).d,
            Vec3(1_f)
        };
    }

    virtual Float pdf_direction(Vec3 wo) const override {
        // Given directions is not samplable if raster position is not in [0,1]^2
        if (!raster_position(wo)) {
            return 0_f;
        }
        return J(wo);
    }

    virtual std::optional<PositionSample> sample_position() const override {
        return PositionSample{
            PointGeometry::make_degenerated(position_),
            Vec3(1_f)
        };
    }

    virtual Float pdf_position(const PointGeometry&) const override {
        return 1_f;
    }

    // --------------------------------------------------------------------------------------------

    virtual std::optional<RaySample> sample_direct(const RaySampleU&, const PointGeometry& geom) const override {
        assert(!geom.infinite);
        const auto geomE = PointGeometry::make_degenerated(position_);
        const auto wo = glm::normalize(geom.p - position_);
        const auto We = Vec3(J(wo));
        const auto p = pdf_direct(geom, geomE, wo);
        if (p == 0_f) {
            return {};
        }
        return RaySample{
            geomE,
            wo,
            We / p
        };
    }

    virtual Float pdf_direct(const PointGeometry& geom, const PointGeometry& geomE, Vec3) const override {
        const auto G = surface::geometry_term(geom, geomE);
        return G == 0_f ? 0_f : 1_f / G;
    }
};

LM_COMP_REG_IMPL(Camera_Pinhole, "camera::pinhole");

LM_NAMESPACE_END(LM_NAMESPACE)
