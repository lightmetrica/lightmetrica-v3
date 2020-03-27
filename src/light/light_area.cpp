/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/light.h>
#include <lm/mesh.h>

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

    virtual void foreach_underlying(const ComponentVisitor& visit) override {
        comp::visit(visit, mesh_);
    }

private:
    Float tranformed_invA(const Transform& transform) const {
        // TODO: Handle degenerated axis
        // e.g., scaling by (.2,.2,.2) leads J=1/5^3
        // but the actual change of areae is J=1/5^2
        return invA_ / transform.J;
    }

    PointGeometry sample_position_on_triangle_mesh(Vec2 up, Float upc, const Transform& transform) const {
        const int i = dist_.sample(upc);
        const auto s = math::safe_sqrt(up[0]);
        const auto tri = mesh_->triangle_at(i);
        const auto a = tri.p1.p;
        const auto b = tri.p2.p;
        const auto c = tri.p3.p;
        const auto p = math::mix_barycentric(a, b, c, Vec2(1_f - s, up[1]*s));
        const auto gn = math::geometry_normal(a, b, c);
        const auto p_trans = Vec3(transform.M * Vec4(p, 1_f));
        const auto gn_trans = glm::normalize(transform.normal_M * gn);
        return PointGeometry::make_on_surface(p_trans, gn_trans, gn_trans);
    }

public:
    virtual void construct(const Json& prop) override {
        Ke_ = json::value<Vec3>(prop, "Ke");
        mesh_ = json::comp_ref<Mesh>(prop, "mesh");
        
        // Construct CDF for surface sampling
        // Note we construct the CDF before transformation
        mesh_->foreach_triangle([&](int, const Mesh::Tri& tri) {
            const auto cr = cross(tri.p2.p - tri.p1.p, tri.p3.p - tri.p1.p);
            dist_.add(math::safe_sqrt(glm::dot(cr, cr)) * .5_f);
        });
        invA_ = 1_f / dist_.c.back();
        dist_.norm();
    }

    // --------------------------------------------------------------------------------------------

    virtual std::optional<RaySample> sample_ray(const RaySampleU& us, const Transform& transform) const override {
        // Sample position
        const auto geomL = sample_position_on_triangle_mesh(us.up, us.upc, transform);
        const auto pA = tranformed_invA(transform);
        
        // Sample direction
        const auto wo_local = math::sample_cosine_weighted(us.ud);
        const auto [u, v] = math::orthonormal_basis(geomL.n);
        const Mat3 to_world(u, v, geomL.n);
        const auto wo_world = to_world * wo_local;
        const auto pD_projSA = math::pdf_cosine_weighted_projSA();
        
        // Contribution & probability
        const auto Le = eval(geomL, wo_world);
        const auto p = pA * pD_projSA;
        const auto contrb = Le / p;

        return RaySample{
            geomL,
            wo_world,
            contrb
        };
    }

    virtual Float pdf_ray(const PointGeometry& geom, Vec3 wo, const Transform& transform) const override {
        const auto pA = pdf_position(geom, transform);
        const auto pD = pdf_direction(geom, wo);
        return pA * pD;
    }

    // --------------------------------------------------------------------------------------------

    virtual std::optional<DirectionSample> sample_direction(const PointGeometry& geom, const DirectionSampleU& us) const override {
        const auto wo_local = math::sample_cosine_weighted(us.ud);
        const auto [u, v] = math::orthonormal_basis(geom.n);
        const Mat3 to_world(u, v, geom.n);
        const auto wo_world = to_world * wo_local;
        const auto pD_projSA = math::pdf_cosine_weighted_projSA();
        const auto Le = eval(geom, wo_world);
        const auto contrb = Le / pD_projSA;
        return DirectionSample{
            wo_world,
            contrb
        };
    }

    virtual Float pdf_direction(const PointGeometry& geom, Vec3 wo) const override {
        if (glm::dot(wo, geom.n) <= 0_f) {
            return 0_f;
        }
        return math::pdf_cosine_weighted_projSA();
    }

    // --------------------------------------------------------------------------------------------

    virtual std::optional<PositionSample> sample_position(const PositionSampleU& us, const Transform& transform) const override {
        const auto geomL = sample_position_on_triangle_mesh(us.up, us.upc, transform);
        const auto pA = tranformed_invA(transform);
        return PositionSample{
            geomL,
            Vec3(1_f / pA)
        };
    }

    virtual Float pdf_position(const PointGeometry&, const Transform& transform) const override {
        return tranformed_invA(transform);
    }

    // --------------------------------------------------------------------------------------------

    virtual std::optional<RaySample> sample_direct(const RaySampleU& us, const PointGeometry& geom, const Transform& transform) const override {
        const auto geomL = sample_position_on_triangle_mesh(us.up, us.upc, transform);
        const auto wo = glm::normalize(geom.p - geomL.p);
        const auto pL = pdf_direct(geom, geomL, transform, wo);
        if (pL == 0_f) {
            return {};
        }
        const auto Le = eval(geomL, wo);
        return RaySample{
            geomL,
            wo,
            Le / pL
        };
    }

    virtual Float pdf_direct(const PointGeometry& geom, const PointGeometry& geomL, const Transform& transform, Vec3 wo) const override {
        if (glm::dot(wo, geomL.n) <= 0_f) {
            return 0_f;
        }
        const auto G = surface::geometry_term(geom, geomL);
        return G == 0_f ? 0_f : tranformed_invA(transform) / G;
    }

    // --------------------------------------------------------------------------------------------

    virtual bool is_infinite() const override {
        return false;
    }

    virtual Vec3 eval(const PointGeometry& geom, Vec3 wo) const override {
        return glm::dot(wo, geom.n) <= 0_f ? Vec3(0_f) : Ke_;
    }
};

LM_COMP_REG_IMPL(Light_Area, "light::area");

LM_NAMESPACE_END(LM_NAMESPACE)
