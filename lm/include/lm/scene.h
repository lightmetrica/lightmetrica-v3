/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "forward.h"
#include "math.h"
#include <variant>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// ----------------------------------------------------------------------------

/*!
    \brief Scene surface point.
    Represents single point on the scene surface, which contains
    geometry information around a point and a weak reference to
    the primitive associated with the point.
*/
struct SurfacePoint {
    int primitive = -1; // Primitive index
    int comp = -1;      // Primitive component index
    bool degenerated;   // Surface is degenerated (e.g., point light)
    Vec3 p;             // Position
    Vec3 n;             // Normal
    Vec2 t;             // Texture coordinates
    Vec3 u, v;          // Orthogonal tangent vectors
    bool endpoint = false;  // Endpoint of light path

    SurfacePoint() {}

    SurfacePoint(Vec3 p)
        : degenerated(true), p(p) {}

    SurfacePoint(Vec3 p, Vec3 n, Vec2 t)
        : SurfacePoint(-1, -1, p, n, t) {}

    SurfacePoint(int primitive, int comp, Vec3 p, Vec3 n, Vec2 t)
        : primitive(primitive), comp(comp), degenerated(false), p(p), n(n), t(t)
    {
        std::tie(u, v) = math::orthonormalBasis(n);
    }

    /*!
        \brief Return true if wi and wo is same direction according to the normal n.
    */
    bool opposite(Vec3 wi, Vec3 wo) const {
        return glm::dot(wi, n) * glm::dot(wo, n) <= 0;
    }
    
    /*!
        \brief Return orthonormal basis according to the incident direction wi.
    */
    std::tuple<Vec3, Vec3, Vec3> orthonormalBasis(Vec3 wi) const {
        const int i = glm::dot(wi, n) > 0;
        return { i ? n : -n, u, i ? v : -v };
    }

    /*!
        \brief Compute geometry term.
    */
    friend Float geometryTerm(const SurfacePoint& s1, const SurfacePoint& s2) {
        Vec3 d = s2.p - s1.p;
        const Float L2 = glm::dot(d, d);
        d = d / sqrt(L2);
        return glm::abs(glm::dot(s1.n, d)) * glm::abs(glm::dot(s2.n, -d)) / L2;
    }
};

// ----------------------------------------------------------------------------

/*!
    \brief Result of sampleRay functions.
*/
struct RaySample {
    SurfacePoint sp;  // Sampled point
    Vec3 wo;          // Sampled direction
    Vec3 weight;      // Contribution divided by probability

    RaySample(const SurfacePoint& sp, Vec3 wo, Vec3 weight)
        : sp(sp), wo(wo), weight(weight) {}

    // Update primitive index and return a reference
    RaySample& asPrimitive(int primitive) {
        sp.primitive = primitive;
        return *this;
    }

    // Update component index and return a reference
    RaySample& asComp(int comp) {
        sp.comp = comp;
        return *this;
    }

    // Update endpoint flag
    RaySample& asEndpoint(bool endpoint) {
        sp.endpoint = endpoint;
        return *this;
    }

    // Multiply weight
    RaySample& multWeight(Float w) {
        weight *= w;
        return *this;
    }

    RaySample(int primitive, int comp, const RaySample& rs)
        : sp(rs.sp), wo(rs.wo), weight(rs.weight)
    {
        sp.primitive = primitive;
        sp.comp = comp;
    }

    // Get a ray from the sample
    Ray ray() const {
        return { sp.p, wo };
    }
};

// ----------------------------------------------------------------------------

/*!
    \brief Scene.
*/
class Scene : public Component {
public:
    /*!
        \brief Load a scene primitive.
    */
    virtual bool loadPrimitive(
        const Component& assetGroup, Mat4 transform, const Json& prop) = 0;

    /*!
        \brief Create primitives from a model.
    */
    virtual bool loadPrimitives(
        const Component& assetGroup, Mat4 transform, const std::string& modelName) = 0;

    /*!
        \brief Iterate triangles in the scene.
    */
    using ProcessTriangleFunc = std::function<
        void(int primitive, int face, Vec3 p1, Vec3 p2, Vec3 p3)>;
    virtual void foreachTriangle(
        const ProcessTriangleFunc& processTriangle) const = 0;

    // ------------------------------------------------------------------------

    /*!
        \brief Build acceleration structure.
    */
    virtual void build(const std::string& name) = 0;

    /*!
        \brief Compute closest intersection point.
    */
    virtual std::optional<SurfacePoint> intersect(
        Ray ray, Float tmin = Eps, Float tmax = Inf) const = 0;

    // ------------------------------------------------------------------------

    /*!
        \brief Check if given surface point is light.
    */
    virtual bool isLight(const SurfacePoint& sp) const = 0;

    /*!
        \brief Check if given surface point is specular.
    */
    virtual bool isSpecular(const SurfacePoint& sp) const = 0;

    // ------------------------------------------------------------------------

    /*!
        \brief Generate a primary ray.
    */
    virtual Ray primaryRay(Vec2 rp) const = 0;

    /*!
        \brief Sample a ray given surface point and incident direction.
        (x,wo) ~ p(x,wo|sp,wi)
    */
    virtual std::optional<RaySample> sampleRay(Rng& rng, const SurfacePoint& sp, Vec3 wi) const = 0;

    /*!
        \brief Sample a ray given pixel position.
        (x,wo) ~ p(x,wo|raster window)
    */
    virtual std::optional<RaySample> samplePrimaryRay(Rng& rng, Vec4 window) const = 0;

    /*!
        \brief Evaluate endpoint contribution.
        f(x,wo) where x is endpoint
    */
    virtual Vec3 evalContrbEndpoint(const SurfacePoint& sp, Vec3 wo) const = 0;
};

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(LM_NAMESPACE)
