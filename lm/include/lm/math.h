/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include  "detail/forward.h"
#include <glm/glm.hpp>
#include <tuple>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// ----------------------------------------------------------------------------

// Default math types deligated to glm library
using Vec2 = glm::tvec2<Float>;
using Vec3 = glm::tvec3<Float>;
using Vec4 = glm::tvec4<Float>;
using Mat3 = glm::tmat3x3<Float>;
using Mat4 = glm::tmat4x4<Float>;

// Floating point literals
constexpr Float operator"" _f(long double v) { return Float(v); }
constexpr Float operator"" _f(unsigned long long v) { return Float(v); }

// Math constants
constexpr Float Inf = 1e+10_f;
constexpr Float Eps = 1e-4_f;
constexpr Float Pi  = 3.14159265358979323846_f;

// ----------------------------------------------------------------------------

/*!
    \brief Ray.
*/
struct Ray {
    Vec3 o;     // Origin
    Vec3 d;     // Direction
};

/*!
    \brief Axis-aligned bounding box
*/
struct Bound {
    Vec3 mi = Vec3(Inf);
    Vec3 ma = Vec3(-Inf);
    Vec3 operator[](int i) const { return (&mi)[i]; }

    // Centroid of the bound
    Vec3 center() const { return (mi + ma) * .5_f; }

    // Surface area of the bound
    Float surfaceArea() const {
        const auto d = ma - mi;
        return 2_f * (d.x * d.y + d.y * d.z + d.z * d.x);
    }

    // Check intersection to the ray
    // http://psgraphics.blogspot.de/2016/02/new-simple-ray-box-test-from-andrew.html
    bool isect(Ray r, Float tmin, Float tmax) const {
        for (int i = 0; i < 3; i++) {
            const Float vd = 1_f / r.d[i];
            auto t1 = (mi[i] - r.o[i]) * vd;
            auto t2 = (ma[i] - r.o[i]) * vd;
            if (vd < 0) {
                std::swap(t1, t2);
            }
            tmin = glm::max(t1, tmin);
            tmax = glm::min(t2, tmax);
            if (tmax < tmin) {
                return false;
            }
        }
        return true;
    }

    // Merges a bound and a point
    friend Bound merge(Bound b, Vec3 p) {
        return { glm::min(b.mi, p), glm::max(b.ma, p) };
    }

    // Merges two bounds
    friend Bound merge(Bound a, Bound b) {
        return { glm::min(a.mi, b.mi), glm::max(a.ma, b.ma) };
    }
};

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(math)

/*!
    \brief Compute orthogonal basis [Duff et al. 2017].
*/
static std::tuple<Vec3, Vec3> orthonormalBasis(Vec3 n) {
    const Float s = copysign(1_f, n.z);
    const Float a = -1_f / (s + n.z);
    const Float b = n.x * n.y * a;
    const Vec3 u(1_f + s * n.x * n.x * a, s * b, -s * n.x);
    const Vec3 v(b, s + n.y * n.y * a, -n.y);
    return { u, v };
}

/*!
    \brief Interpolation with barycentric coordinates.
*/
template <typename T>
static T mixBarycentric(T a, T b, T c, Vec2 uv) {
    return a * (1_f - uv.x - uv.y) + b * uv.x + c * uv.y;
}

LM_NAMESPACE_END(math)

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(LM_NAMESPACE)
