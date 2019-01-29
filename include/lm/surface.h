/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "math.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup scene
    @{
*/

/*!
    \brief Geometry information of a point inside the scene.
*/
struct PointGeometry {
    bool degenerated;       // True if surface is degenerated (e.g., point light)
    bool infinite;          // True if the point is a point at infinity
    Vec3 p;                 // Position
    union {
        Vec3 n;             // Normal
        Vec3 wo;            // Direction from a point at infinity
    };
    Vec2 t;                 // Texture coordinates
    Vec3 u, v;              // Orthogonal tangent vectors
    //bool endpoint = false;  // Endpoint of light path

    /*!
        \brief Make degenerated point.
        \param p Position.
    */
    static PointGeometry makeDegenerated(Vec3 p) {
        PointGeometry geom;
        geom.degenerated = true;
        geom.infinite = false;
        geom.p = p;
        return geom;
    }

    /*!
        \brief Make a point at infinity.
        \param wo Direction from a point at infinity.
    */
    static PointGeometry makeInfinite(Vec3 wo) {
        PointGeometry geom;
        geom.degenerated = false;
        geom.infinite = true;
        geom.wo = wo;
        return geom;
    }

    /*!
        \brief Make a point on surface.
        \param p Position.
        \param n Normal.
        \param t Texture coordinates.
    */
    static PointGeometry makeOnSurface(Vec3 p, Vec3 n, Vec3 t) {
        PointGeometry geom;
        geom.degenerated = false;
        geom.infinite = false;
        geom.p = p;
        geom.n = n;
        geom.t = t;
        std::tie(geom.u, geom.v) = math::orthonormalBasis(n);
        return geom;
    }

    /*!
        \brief Make a point on surface.
        \param p Position.
        \param n Normal.
    */
    static PointGeometry makeOnSurface(Vec3 p, Vec3 n) {
        return makeOnSurface(p, n, {});
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
};

LM_NAMESPACE_BEGIN(surface)

/*!
    \brief Compute geometry term.
*/
static Float geometryTerm(const PointGeometry& s1, const PointGeometry& s2) {
    Vec3 d = s2.p - s1.p;
    const Float L2 = glm::dot(d, d);
    d = d / std::sqrt(L2);
    return glm::abs(glm::dot(s1.n, d)) * glm::abs(glm::dot(s2.n, -d)) / L2;
}

LM_NAMESPACE_END(surface)

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
