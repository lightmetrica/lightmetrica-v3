/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "math.h"
#include "surface.h"
#include <variant>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// ----------------------------------------------------------------------------

/*!
    \addtogroup scene
    @{
*/

/*!
    \brief Scene surface point.
    Represents single point on the scene surface, which contains
    geometry information around a point and a weak reference to
    the primitive associated with the point.
*/
struct SurfacePoint {
    int primitive = -1;   // Primitive index
    int comp = -1;        // Primitive component index
    PointGeometry geom;   // Surface point geometry information

    //static SurfacePoint makeEndpoint() {
    //    
    //}

    //SurfacePoint(Vec3 p)
    //    : degenerated(true), p(p) {}

    //SurfacePoint(Vec3 p, Vec3 n, Vec2 t)
    //    : SurfacePoint(-1, -1, p, n, t) {}

    //SurfacePoint(int primitive)
    //    : primitive(primitive), degenerated(false), endpoint(true) {}

    //SurfacePoint(int primitive, int comp, Vec3 p, Vec3 n, Vec2 t)
    //    : primitive(primitive), comp(comp), degenerated(false), p(p), n(n), t(t)
    //{
    //    std::tie(u, v) = math::orthonormalBasis(n);
    //}
};

// ----------------------------------------------------------------------------

/*!
    \brief Result of sampleRay functions.
*/
struct RaySample {
    PointGeometry geom;   // Sampled geometry information
    Vec3 wo;                // Sampled direction
    int comp;               // Sampled component index
    Vec3 weight;            // Contribution divided by probability

    //RaySample(const SurfacePoint& sp, Vec3 wo, Vec3 weight)
    //    : sp(sp), wo(wo), weight(weight) {}

    // Update primitive index and return a reference
    //RaySample& asPrimitive(int primitive) {
    //    sp.primitive = primitive;
    //    return *this;
    //}

    // Update component index and return a reference
    //RaySample& asComp(int comp) {
    //    sp.comp = comp;
    //    return *this;
    //}

    // Update endpoint flag
    //RaySample& asEndpoint(bool endpoint) {
    //    sp.endpoint = endpoint;
    //    return *this;
    //}

    // Multiply weight
    //RaySample& multWeight(Float w) {
    //    weight *= w;
    //    return *this;
    //}

    //RaySample(int primitive, int comp, const RaySample& rs)
    //    : sp(rs.sp), wo(rs.wo), weight(rs.weight)
    //{
    //    sp.primitive = primitive;
    //    sp.comp = comp;
    //}

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
        \param name Name of the acceleration structure.
        \param prop Property for configuration.
    */
    virtual void build(const std::string& name, const Json& prop) = 0;

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
        \rst
        (x,wo) ~ p(x,wo|sp,wi)
        \endrst
    */
    virtual std::optional<RaySample> sampleRay(Rng& rng, const SurfacePoint& sp, Vec3 wi) const = 0;

    /*!
        \brief Sample a ray given pixel position.
        \rst
        (x,wo) ~ p(x,wo|raster window)
        \endrst
    */
    virtual std::optional<RaySample> samplePrimaryRay(Rng& rng, Vec4 window) const = 0;

    /*!
        \brief Sample a position on a light.
    */
    virtual std::optional<LightSample> sampleLight(Rng& rng, const SurfacePoint& sp) const = 0;

    // ------------------------------------------------------------------------

    /*!
        \brief Evaluate extended BSDF.
    */
    virtual Vec3 evalBsdf(const SurfacePoint& sp, Vec3 wi, Vec3 wo) const = 0;

    /*!
        \brief Evaluate endpoint contribution.
        \rst
        f(x,wo) where x is endpoint
        \endrst
    */
    virtual Vec3 evalContrbEndpoint(const SurfacePoint& sp, Vec3 wo) const = 0;

    /*!
        \brief Evaluate reflectance (if available).
        \rst
        \rho(x)
        \endrst
    */
    virtual std::optional<Vec3> reflectance(const SurfacePoint& sp) const = 0;
};

/*!
    @}
*/

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(LM_NAMESPACE)
