/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "detail/component.h"
#include "detail/forward.h"
#include "math.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// ----------------------------------------------------------------------------

/*!
    \brief Scene surface point.
    Geometry information around a scene surface point.
*/
struct SurfacePoint {
    Vec3 p;         // Position
    Vec3 n;         // Normal
    Vec2 t;         // Texture coordinates
    Vec3 u, v;      // Orthogonal tangent vectors

    SurfacePoint() {}
    SurfacePoint(Vec3 p, Vec3 n, Vec3 t) : p(p), n(n), t(t) {
        std::tie(u, v) = math::orthonormalBasis(n);
    }

    /*!
        \brief Returns true if wi and wo is same direction according to the normal n.
    */
    bool opposite(Vec3 wi, Vec3 wo) const {
        return glm::dot(wi, n) * glm::dot(wo, n) <= 0;
    }
    
    /*!
        \brief Returns orthonormal basis according to the incident direction wi.
    */
    std::tuple<Vec3, Vec3, Vec3> orthonormalBasis(Vec3 wi) const {
        const int i = glm::dot(wi, n) > 0;
        return { i ? n : -n, u, i ? v : -v };
    }
};

/*!
    \brief Compute geometry term.
*/
Float geometryTerm(const SurfacePoint& s1, const SurfacePoint& s2) {
    Vec3 d = s2.p - s1.p;
    const Float L2 = glm::dot(d, d);
    d = d / sqrt(L2);
    return glm::abs(glm::dot(s1.n, d)) * glm::abs(glm::dot(s2.n, -d)) / L2;
}

// ----------------------------------------------------------------------------

/*!
    \brief Primitive.
    Primitive component of the scene.
*/
struct Primitive : public Component {
    Mat4 transform;
    const Component* mesh = nullptr;
    const Component* material = nullptr;
    const Component* sensor = nullptr;
    const Component* light = nullptr;
};

/*!
    \brief Scene.
*/
class Scene : public Component {
public:
    /*!
        \brief Loads a scene primitive.
    */
    virtual bool loadPrimitive(const std::string& name, const Assets& assets,
        Mat4 transform, const Json& prop) = 0;

    /*!
        \brief Build acceleration structure.
    */
    virtual void build(const std::string& name) = 0;

    /*!
        \brief Hit information.
    */
    struct Hit {
        SurfacePoint sp;
        const Primitive* p;
    };

    /*!
        \brief Compute closest intersection point.
    */
    virtual std::optional<Hit> intersect(const Ray& ray, Float tmin = Eps, Float tmax = Inf) = 0;
};

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(LM_NAMESPACE)
