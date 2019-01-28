/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include  "common.h"

#pragma warning(push)
#pragma warning(disable:4201)  // nonstandard extension used: nameless struct/union
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/transform.hpp>
#pragma warning(pop)

#include <tuple>
#include <optional>
#include <random>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup math
    @{
*/

// ----------------------------------------------------------------------------

// Default math types deligated to glm library
using Vec2 = glm::tvec2<Float>;     //!< 2d vector
using Vec3 = glm::tvec3<Float>;     //!< 3d vector
using Vec4 = glm::tvec4<Float>;     //!< 4d vector
using Mat3 = glm::tmat3x3<Float>;   //!< 3x3 matrix
using Mat4 = glm::tmat4x4<Float>;   //!< 4x4 matrix

// Floating point literals
LM_NAMESPACE_BEGIN(literals)
constexpr Float operator"" _f(long double v) { return Float(v); }
constexpr Float operator"" _f(unsigned long long v) { return Float(v); }
LM_NAMESPACE_END(literals)

// Make user-defined literals default inside lm namespace
using namespace literals;

// Math constants
constexpr Float Inf = 1e+10_f;                  //!< Big number 
constexpr Float Eps = 1e-4_f;                   //!< Error tolerance 
constexpr Float Pi  = 3.14159265358979323846_f; //!< Value of Pi

// ----------------------------------------------------------------------------

/*!
    \brief Ray.
*/
struct Ray {
    Vec3 o;     //!< Origin
    Vec3 d;     //!< Direction
};

/*!
    \brief Axis-aligned bounding box
*/
struct Bound {
    Vec3 mi = Vec3(Inf);    //!< Minimum coordinates
    Vec3 ma = Vec3(-Inf);   //!< Maximum coordinates
    
    template <typename Archive>
    void serialize(Archive& ar) {
        ar(mi, ma);
    }

    /*!
        \brief Index operator.
        \return 0: minimum coordinates, 1: maximum coordinates.
    */
    Vec3 operator[](int i) const { return (&mi)[i]; }

    /*!
        \brief Return centroid of the bound.
        \return Centroid.
    */
    Vec3 center() const { return (mi + ma) * .5_f; }

    /*!
        \brief Surface area of the bound.
        \return Surface area.
    */
    Float surfaceArea() const {
        const auto d = ma - mi;
        return 2_f * (d.x * d.y + d.y * d.z + d.z * d.x);
    }

    /*!
        \brief Check intersection to the ray.
        \param r Ray.
        \param tmin Minimum valid range along with the ray from origin.
        \param tmax Maximum valid range along with the ray from origin.
        
        \rst
        Floating point exceptions must be disabled because for performance
        the function facilitates operations on Inf or NaN (`ref`_).

        .. _ref: http://psgraphics.blogspot.de/2016/02/new-simple-ray-box-test-from-andrew.html
        \endrst
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
        \brief Merge a bound and a point.
        \param b Bound.
        \param p Point.
        \return Merged bound.
    */
    friend Bound merge(Bound b, Vec3 p) {
        return { glm::min(b.mi, p), glm::max(b.ma, p) };
    }

    /*!
        \brief Merge two bounds.
        \param a Bound 1.
        \param b Bound 2.
        \return Merged bound.
    */
    friend Bound merge(Bound a, Bound b) {
        return { glm::min(a.mi, b.mi), glm::max(a.ma, b.ma) };
    }
};

/*!
    @}
*/

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
    \addtogroup math
    @{
*/

/*!
    \brief Random number generator.
    
    \rst
    The class provides the feature of uniform random number generator.
    Various random variables are defined based on the uniform random number
    generated by this class. Note that the class internally holds the state
    therefore the member function calls are `not` thread-safe.

    .. We manually documented the member functions
       because doxygen is not good at documenting template specializations.

    **Public Members**

    .. cpp:function:: Rng(int seed)

       Construct the random number generator by a given seed value.

    .. cpp:function:: Float u()

       Generate an uniform random number in [0,1).
    \endrst
*/
using Rng = detail::RngImpl<Float>;

// ----------------------------------------------------------------------------

/*!
    \brief 1d discrete distribution.
*/
struct Dist {
    std::vector<Float> c{ 0_f }; // CDF

    template <typename Archive>
    void serialize(Archive& ar) {
        ar(c);
    }

    /*!
        \brief Add a value to the distribution.
        \param v Value to be added.
    */
    void add(Float v) {
        c.push_back(c.back() + v);
    }

    /*!
        \brief Normalize the distribution.
    */
    void norm() {
        const auto sum = c.back();
        for (auto& v : c) {
            v /= sum;
        }
    }

    /*!
        \brief Evaluate pmf.
        \param i Index.
        \return Evaluated pmf.
    */
    Float p(int i) const {
        return (i < 0 || i + 1 >= int(c.size())) ? 0 : c[i + 1] - c[i];
    }

    /*!
        \brief Sample from the distribution.
        \param rn Random number generator.
        \return Sampled index.
    */
    int samp(Rng& rn) const {
        const auto it = std::upper_bound(c.begin(), c.end(), rn.u());
        return std::clamp(int(std::distance(c.begin(), it)) - 1, 0, int(c.size()) - 2);
    }
};

// ----------------------------------------------------------------------------

/*!
    \brief 2d discrete distribution.
*/
struct Dist2 {
    std::vector<Dist> ds;   // Conditional distribution correspoinding to a row
    Dist m;                 // Marginal distribution
    int w, h;               // Size of the distribution

    template <typename Archive>
    void serialize(Archive& ar) {
        ar(ds, m, w, h);
    }

    /*!
        \brief Add values to the distribution.
        \param v Values to be added.
        \param cols Number of columns.
        \param rows Number of rows.
    */
    void init(const std::vector<Float>& v, int cols, int rows) {
        w = cols;
        h = rows;
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

    /*!
        \brief Evaluate pmf.
        \param u Index in column.
        \param v Index in row.
        \return Evaluated pmf.
    */
    Float p(Float u, Float v) const {
        const int y = std::min(int(v * h), h - 1);
        return m.p(y) * ds[y].p(int(u * w)) * w * h;
    }

    /*!
        \brief Sample from the distribution.
        \param rn Random number generator.
        \return Sampled position.
    */
    Vec2 samp(Rng& rn) const {
        const int y = m.samp(rn);
        const int x = ds[y].samp(rn);
        return Vec2((x + rn.u()) / w, (y + rn.u()) / h);
    }
};

/*!
    @}
*/

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(math)

/*!
    \addtogroup math
    @{
*/

/*!
    \brief Compute orthogonal basis.
    \param n Normal vector.
    
    \rst
    This implementation is based on the technique by Duff et al. [Duff2017]_.

    .. [Duff2017] Duff et al., Building an Orthonormal Basis, Revisited., JCGT, 2017.
    \endrst
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
    \param a A value associated with the first point on the triangle.
    \param b A value associated with the second point on the triangle.
    \param c A value associated with the third point on the triangle.
    \param uv Ratio of interpolation in [0,1]^2.
    \return Interpolated value.
*/
template <typename VecT>
static VecT mixBarycentric(VecT a, VecT b, VecT c, Vec2 uv) {
    return a * (1_f - uv.x - uv.y) + b * uv.x + c * uv.y;
}

/*!
    \brief Check if components of a vector are all zeros.
    \param v A vector.
    \return ``true`` if components are all zeros, ``false`` otherwise.
*/
template <typename VecT>
static bool isZero(VecT v) {
    return glm::all(glm::equal(v, VecT(0_f)));
}

/*!
    \brief Square root handling possible negative input due to the rounding error.
    \param v Value.
    \return Square root of the input value.
*/
static Float safeSqrt(Float v) {
    return std::sqrt(std::max(0_f, v));
}

/*!
    \brief Compute square.
    \param v Value.
    \return Square of the input value.
*/
static Float sq(Float v) {
    return v * v;
}

/*!
    \brief Reflected direction.
    \param w Incident direction.
    \param n Surface normal.
    \return Reflected direction.
*/
static Vec3 reflection(Vec3 w, Vec3 n) {
    return 2_f * glm::dot(w, n) * n - w;
}

/*!
    \brief Refracted direction.
    \param wi Incident direction.
    \param n Surface normal.
    \param eta Relative ior.
    \return Refracted direction.
*/
static std::optional<Vec3> refraction(Vec3 wi, Vec3 n, Float eta) {
    const auto t = glm::dot(wi, n);
    const auto t2 = 1_f - eta*eta*(1_f-t*t);
    return t2 > 0_f ? eta*(n*t-wi)-n*safeSqrt(t2) : std::optional<Vec3>{};
}

/*!
    \brief Cosine-weighted direction sampling.
    \param rng Random number generator.
    \return Sampled value.
*/
static Vec3 sampleCosineWeighted(Rng& rng) {
    const auto r = safeSqrt(rng.u());
    const auto t = 2_f * Pi * rng.u();
    const auto x = r * std::cos(t);
    const auto y = r * std::sin(t);
    return { x, y, safeSqrt(1_f - x*x - y*y) };
}

/*!
    @}
*/

LM_NAMESPACE_END(math)

// ----------------------------------------------------------------------------

/*!
    \addtogroup math
    @{
*/

/*!
    \brief Transform.
*/
struct Transform {
    Mat4 M;                //!< Transform associated to the primitive
    Mat3 normalM;        //!< Transform for normals
    Float J;            //!< J := |det(M_lin)| where M_lin is linear component of M

    template <typename Archive>
    void serialize(Archive& ar) {
        ar(M, normalM, J);
    }

    Transform() = default;

    /*!
        \brief Construct the transform with 4x4 transformation matrix.
        \param M Transformation matrix.
    */
    Transform(const Mat4& M) : M(M) {
        normalM = Mat3(glm::transpose(glm::inverse(M)));
        J = std::abs(glm::determinant(Mat3(M)));
    }
};

/*!
    @}
*/

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(LM_NAMESPACE)
