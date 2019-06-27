/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/light.h>
#include <lm/mesh.h>
#include <lm/json.h>
#include <lm/user.h>
#include <lm/serial.h>
#include <lm/surface.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*
\rst
.. function:: light::area

   Area light.

   :param color Ke: Luminance.
   :param str mesh: Underlying mesh specified by asset name or locator.
\endrst
*/
class Light_Area final : public Light {
private:
    Vec3 Ke_;     // Luminance
    Dist dist_;   // For surface sampling of area lights
    Float invA_;  // Inverse area of area lights
    Mesh* mesh_;  // Underlying mesh

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(Ke_, dist_, invA_, mesh_);
    }

    virtual void foreachUnderlying(const ComponentVisitor& visit) override {
        comp::visit(visit, mesh_);
    }

private:
    Float tranformedInvA(const Transform& transform) const {
        // TODO: Handle degenerated axis
        // e.g., scaling by (.2,.2,.2) leads J=1/5^3
        // but the actual change of areae is J=1/5^2
        return invA_ / transform.J;
    }

public:
    virtual bool construct(const Json& prop) override {
        Ke_ = prop["Ke"];
        mesh_ = comp::get<Mesh>(prop["mesh"]);
        if (!mesh_) {
            return false;
        }
        
        // Construct CDF for surface sampling
        // Note we construct the CDF before transformation
        mesh_->foreachTriangle([&](int, const Mesh::Tri& tri) {
            const auto cr = cross(tri.p2.p - tri.p1.p, tri.p3.p - tri.p1.p);
            dist_.add(math::safeSqrt(glm::dot(cr, cr)) * .5_f);
        });
        invA_ = 1_f / dist_.c.back();
        dist_.norm();

        return true;
    }

    virtual std::optional<LightRaySample> sample(Rng& rng, const PointGeometry& geom, const Transform& transform) const override {
        const int i = dist_.samp(rng);
        const auto s = math::safeSqrt(rng.u());
        const auto tri = mesh_->triangleAt(i);
        const auto a = tri.p1.p;
        const auto b = tri.p2.p;
        const auto c = tri.p3.p;
        const auto p = math::mixBarycentric(a, b, c, Vec2(1_f-s, rng.u()*s));
        const auto n = glm::normalize(glm::cross(b - a, c - a));
        const auto geomL = PointGeometry::makeOnSurface(
            transform.M * Vec4(p, 1_f),
            transform.normalM * n);
        const auto ppL = geomL.p - geom.p;
        const auto wo = glm::normalize(ppL);
        const auto pL = pdf(geom, geomL, 0, transform, -wo);
        if (pL == 0_f) {
            return {};
        }
        const auto Le = eval(geomL, 0, -wo);
        return LightRaySample{
            geomL,
            -wo,
            0,
            Le / pL
        };
    }

    virtual Float pdf(const PointGeometry& geom, const PointGeometry& geomL, int, const Transform& transform, Vec3) const override {
        const auto G = surface::geometryTerm(geom, geomL);
        return G == 0_f ? 0_f : tranformedInvA(transform) / G;
    }

    virtual bool isSpecular(const PointGeometry&, int) const override {
        return false;
    }

    virtual bool isInfinite() const override {
        return false;
    }

    virtual Vec3 eval(const PointGeometry& geom, int, Vec3 wo) const override {
        return glm::dot(wo, geom.n) <= 0_f ? Vec3(0_f) : Ke_;
    }
};

LM_COMP_REG_IMPL(Light_Area, "light::area");

LM_NAMESPACE_END(LM_NAMESPACE)
