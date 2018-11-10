/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include  "common.h"

#pragma warning(push)
#pragma warning(disable:4201)  // nonstandard extension used: nameless struct/union
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>
#pragma warning(pop)

#include <tuple>
#include <optional>
#include <random>

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

    /*!
        \brief Centroid of the bound.
    */
    Vec3 center() const { return (mi + ma) * .5_f; }

    /*!
        \brief Surface area of the bound.
    */
    Float surfaceArea() const {
        const auto d = ma - mi;
        return 2_f * (d.x * d.y + d.y * d.z + d.z * d.x);
    }

    /*!
        \brief Check intersection to the ray.
        Floating point exceptions must be disabled because for performance
        the function facilitates operations on Inf or NaN.
        cf. http://psgraphics.blogspot.de/2016/02/new-simple-ray-box-test-from-andrew.html
    */
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

    /*!
        \brief Merges a bound and a point.
    */
    friend Bound merge(Bound b, Vec3 p) {
        return { glm::min(b.mi, p), glm::max(b.ma, p) };
    }

    /*!
        Merges two bounds.
    */
    friend Bound merge(Bound a, Bound b) {
        return { glm::min(a.mi, b.mi), glm::max(a.ma, b.ma) };
    }
};

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(detail)

class RngImplBase {
private:
    std::mt19937 eng;
    std::uniform_real_distribution<double> dist;  // Always use double

protected:
    RngImplBase() = default;
    RngImplBase(int seed) {
        eng.seed(seed);
        dist.reset();
    }
    double u() { return dist(eng); }
};

template <typename F>
class RngImpl;

template <>
class RngImpl<double> : public RngImplBase {
public:
    RngImpl() = default;
    RngImpl(int seed) : RngImplBase(seed) {}
    using RngImplBase::u;
};

template <>
class RngImpl<float> : public RngImplBase {
public:
    RngImpl() = default;
    RngImpl(int seed) : RngImplBase(seed) {}

    /*
        According to the C++ standard std::uniform_real_distribution::operator()
        always returns non-inclusive range. For instance, in our case u()
        should return values in [0,1). Yet due to the rounding issue
        the function might return exact value of 1.f when std::uniform_real_distribution is
        specialized for float. To avoid this problem, we generate
        the number with double then round it toward negative infinity
        rather than generating the number with float directly.
        cf. https://stackoverflow.com/questions/25668600/is-1-0-a-valid-output-from-stdgenerate-canonical
    */
    float u() {
        const double rd = RngImplBase::u();
        float rf = static_cast<float>(rd);
        if (rf > rd) {
            rf = std::nextafter(rf, -std::numeric_limits<float>::infinity());
        }
        assert(rf < 1.f);
        return rf;
    }
};

LM_NAMESPACE_END(detail)

/*!
    \brief Random number generator.
*/
using Rng = detail::RngImpl<Float>;

// ----------------------------------------------------------------------------

/*!
    1d discrete distribution.
*/
struct Dist {
    std::vector<Float> c{ 0_f }; // CDF

    // Add a value to the distribution
    void add(Float v) {
        c.push_back(c.back() + v);
    }

    // Normalize the distribution
    void norm() {
        const auto sum = c.back();
        for (auto& v : c) {
            v /= sum;
        }
    }

    // Evaluate pmf
    Float p(int i) const {
        return (i < 0 || i + 1 >= int(c.size())) ? 0 : c[i + 1] - c[i];
    }

    // Sample from the distribution
    int samp(Rng& rn) const {
        const auto it = std::upper_bound(c.begin(), c.end(), rn.u());
        return std::clamp(int(std::distance(c.begin(), it)) - 1, 0, int(c.size()) - 2);
    }
};

// ----------------------------------------------------------------------------

// 2d discrete distribution
struct Dist2 {
    std::vector<Dist> ds;   // Conditional distribution correspoinding to a row
    Dist m;                 // Marginal distribution
    int w, h;               // Size of the distribution

    // Add values to the distribution
    void init(const std::vector<Float>& v, int a, int b) {
        w = a;
        h = b;
        ds.assign(h, {});
        for (int i = 0; i < h; i++) {
            auto& d = ds[i];
            for (int j = 0; j < w; j++) {
                d.add(v[i * w + j]);
            }
            m.add(d.c.back());
            d.norm();
        }
        m.norm();
    }

    // Evaluate pmf
    Float p(Float u, Float v) const {
        const int y = std::min(int(v * h), h - 1);
        return m.p(y) * ds[y].p(int(u * w)) * w * h;
    }

    // Sample from the distribution
    std::tuple<Float, Float> samp(Rng& rn) const {
        const int y = m.samp(rn);
        const int x = ds[y].samp(rn);
        return {(x + rn.u()) / w, (y + rn.u()) / h};
    }
};

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(math)

/*!
    \brief Compute orthogonal basis [Duff et al. 2017].
*/
static std::tuple<Vec3, Vec3> orthonormalBasis(Vec3 n) {
    const auto s = copysign(1_f, n.z);
    const auto a = -1_f / (s + n.z);
    const auto b = n.x * n.y * a;
    const Vec3 u(1_f + s * n.x * n.x * a, s * b, -s * n.x);
    const Vec3 v(b, s + n.y * n.y * a, -n.y);
    return { u, v };
}

/*!
    \brief Interpolation with barycentric coordinates.
*/
template <typename VecT>
static VecT mixBarycentric(VecT a, VecT b, VecT c, Vec2 uv) {
    return a * (1_f - uv.x - uv.y) + b * uv.x + c * uv.y;
}

/*!
    \brief Check if a vector only contains exactly components of zeros.
*/
template <typename VecT>
static bool isZero(VecT v) {
    return glm::all(glm::equal(v, VecT(0_f)));
}

/*!
    \brief Square root handling possible negative input due to the rounding error.
*/
static Float safeSqrt(Float v) {
    return std::sqrt(std::max(0_f, v));
}

/*!
    \brief Compute square.
*/
static Float sq(Float v) {
    return v * v;
}

/*!
    \brief Reflected direction.
*/
static Vec3 reflection(Vec3 w, Vec3 n) {
    return 2_f * glm::dot(w, n) * n - w;
}

/*!
    \brief Refracted direction.
*/
static std::optional<Vec3> refraction(Vec3 wi, Vec3 n, Float eta) {
    const auto t = glm::dot(wi, n);
    const auto t2 = 1_f - eta*eta*(1_f-t*t);
    return t2 > 0_f ? eta*(n*t-wi)-n*safeSqrt(t2) : std::optional<Vec3>{};
}

/*!
    \brief Cosine-weighted direction sampling.
*/
static Vec3 sampleCosineWeighted(Rng& rng) {
    const auto r = safeSqrt(rng.u());
    const auto t = 2_f * Pi * rng.u();
    const auto x = r * std::cos(t);
    const auto y = r * std::sin(t);
    return { x, y, safeSqrt(1_f - x*x - y*y) };
}

LM_NAMESPACE_END(math)

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(LM_NAMESPACE)
