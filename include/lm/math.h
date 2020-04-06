/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include  "common.h"

#pragma warning(push)
#pragma warning(disable:4201)  // nonstandard extension used: nameless struct/union
#pragma warning(disable:4127)  // conditional expression is constant
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#pragma warning(pop)

#include <tuple>
#include <optional>
#include <random>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup math
    @{
*/

// ------------------------------------------------------------------------------------------------

#pragma region Basic types and constants

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

#pragma endregion

// ------------------------------------------------------------------------------------------------

#pragma region Ray and bound

/*!
    \brief Ray.
*/
struct Ray {
    Vec3 o;     //!< Origin
    Vec3 d;     //!< Direction

    //! \cond
    template <typename Archive>
    void serialize(Archive& ar) {
        ar(o, d);
    }
    //! \endcond
};

/*!
    \brief Axis-aligned bounding box
*/
struct Bound {
    Vec3 min = Vec3(Inf);    //!< Minimum coordinates
    Vec3 max = Vec3(-Inf);   //!< Maximum coordinates
    
    //! \cond
    template <typename Archive>
    void serialize(Archive& ar) {
        ar(min, max);
    }
    //! \endcond

    /*!
        \brief Index operator.
        \return 0: minimum coordinates, 1: maximum coordinates.
    */
    Vec3 operator[](int i) const { return (&min)[i]; }

    /*!
        \brief Return centroid of the bound.
        \return Centroid.
    */
    Vec3 center() const { return (min + max) * .5_f; }

    /*!
        \brief Surface area of the bound.
        \return Surface area.
    */
    Float surface_area() const {
        const auto d = max - min;
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
        return isect_range(r, tmin, tmax);
    }

    /*!
        \brief Check intersection to the ray and update tmin and tmax.
        \param r Ray.
        \param tmin Minimum valid range along with the ray from origin.
        \param tmax Maximum valid range along with the ray from origin.

        \rst
        This function checks intersection between ray and bound
        and assign the intersected range to tmin and tmax
        if the ray intersects the bound.
        \endrst
    */
    bool isect_range(Ray r, Float& tmin, Float& tmax) const {
        for (int i = 0; i < 3; i++) {
            const Float vd = 1_f / r.d[i];
            auto t1 = (min[i] - r.o[i]) * vd;
            auto t2 = (max[i] - r.o[i]) * vd;
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
        return { glm::min(b.min, p), glm::max(b.max, p) };
    }

    /*!
        \brief Merge two bounds.
        \param a Bound 1.
        \param b Bound 2.
        \return Merged bound.
    */
    friend Bound merge(Bound a, Bound b) {
        return { glm::min(a.min, b.min), glm::max(a.max, b.max) };
    }
};

/*!
    \brief Sphere bound.
*/
struct SphereBound {
    Vec3 center;        //!< Center of the sphere.
    Float radius;       //!< Radius of the sphere.

    //! \cond
    template <typename Archive>
    void serialize(Archive& ar) {
        ar(center, radius);
    }
    //! \endcond
};

#pragma endregion

// ------------------------------------------------------------------------------------------------

#pragma region Random number generator

LM_NAMESPACE_BEGIN(detail)

class RngImplBase {
private:
    std::mt19937 eng_;
    std::uniform_real_distribution<double> dist_;  // Always use double
    std::uniform_int_distribution<int> dist_int_;

protected:
    RngImplBase() {
        // Initialize eng_ with random_device
        eng_.seed(std::random_device{}());
    }
    RngImplBase(int seed) {
        eng_.seed(seed);
    }
    double u() { return dist_(eng_); }
    int u_int() { return dist_int_(eng_); }
};

template <typename F>
class RngImpl;

template <>
class RngImpl<double> : public RngImplBase {
public:
    RngImpl() = default;
    RngImpl(int seed) : RngImplBase(seed) {}
    using RngImplBase::u;
    using RngImplBase::u_int;

    template <typename T>
    T next() {
        T us;
        const int N = sizeof(T) / sizeof(double);
        for (int i = 0; i < N; i++) {
            *(reinterpret_cast<double*>(&us) + i) = u();
        }
        return us;
    }
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

    using RngImplBase::u_int;
};

LM_NAMESPACE_END(detail)


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

#pragma endregion

// ------------------------------------------------------------------------------------------------

#pragma region Discrete distributions

/*!
    \brief 1d discrete distribution.
*/
struct Dist {
    std::vector<Float> c{ 0_f }; // CDF

    //! \cond
    template <typename Archive>
    void serialize(Archive& ar) {
        ar(c);
    }
    //! \endcond

    /*!
        \brief Clear internal state.
    */
    void clear() {
        c.clear();
        c.push_back(0_f);
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
    Float pmf(int i) const {
        return (i < 0 || i + 1 >= int(c.size())) ? 0 : c[i + 1] - c[i];
    }

    /*!
        \brief Sample from the distribution.
        \param rn Random number generator.
        \return Sampled index.
    */
    int sample(Float u) const {
        const auto it = std::upper_bound(c.begin(), c.end(), u);
        return std::clamp(int(std::distance(c.begin(), it)) - 1, 0, int(c.size()) - 2);
    }
};

// ------------------------------------------------------------------------------------------------

/*!
    \brief 2d discrete distribution.
*/
struct Dist2 {
    std::vector<Dist> ds;   // Conditional distribution correspoinding to a row
    Dist m;                 // Marginal distribution
    int w, h;               // Size of the distribution

    //! \cond
    template <typename Archive>
    void serialize(Archive& ar) {
        ar(ds, m, w, h);
    }
    //! \endcond

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
    Float pdf(Float u, Float v) const {
        const int y = std::min(int(v * h), h - 1);
        return m.pmf(y) * ds[y].pmf(int(u * w)) * w * h;
    }

    /*!
        \brief Sample from the distribution.
        \param rn Random number generator.
        \return Sampled position.
    */
    Vec2 sample(Vec4 u) const {
        const int y = m.sample(u[0]);
        const int x = ds[y].sample(u[1]);
        return Vec2((x + u[2]) / w, (y + u[3]) / h);
    }
};

#pragma endregion

/*!
    @}
*/

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(math)

/*!
    \addtogroup math
    @{
*/

#pragma region Random number seed

/*!
    \brief Generate random number for seed.
    \return Random seed.
*/
static unsigned int rng_seed() {
    return std::random_device{}();
}

#pragma endregion

#pragma region Basic math functions

/*!
    \brief Check if components of a vector are all zeros.
    \param v A vector.
    \return ``true`` if components are all zeros, ``false`` otherwise.
*/
template <typename VecT>
static bool is_zero(VecT v) {
    return glm::all(glm::equal(v, VecT(0_f)));
}

/*!
    \brief Square root handling possible negative input due to the rounding error.
    \param v Value.
    \return Square root of the input value.
*/
static Float safe_sqrt(Float v) {
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
    \brief Result of refraction() function.
*/
struct RefractionResult {
    Vec3 wt;        // Refracted direction
    bool total;     // True if total internal reflection happens
};

/*!
    \brief Refracted direction.
    \param wi Incident direction.
    \param n Surface normal.
    \param eta Relative ior.
    \return Refracted direction.
*/
static RefractionResult refraction(Vec3 wi, Vec3 n, Float eta) {
    const auto t = glm::dot(wi, n);
    const auto t2 = 1_f - eta*eta*(1_f-t*t);
    if (t2 <= 0_f) {
        return {{}, true};
    }
    return {eta*(n*t-wi)-n*safe_sqrt(t2), false};
}

#pragma endregion

#pragma region Geometry related

/*!
    \brief Compute orthogonal basis.
    \param n Normal vector.
    
    \rst
    This implementation is based on the technique by Duff et al. [Duff2017]_.

    .. [Duff2017] Duff et al., Building an Orthonormal Basis, Revisited., JCGT, 2017.
    \endrst
*/
static std::tuple<Vec3, Vec3> orthonormal_basis(Vec3 n) {
    const auto s = copysign(1_f, n.z);
    const auto a = -1_f / (s + n.z);
    const auto b = n.x * n.y * a;
    const Vec3 u(1_f + s * n.x * n.x * a, s * b, -s * n.x);
    const Vec3 v(b, s + n.y * n.y * a, -n.y);
    return { u, v };
}

/*!
    \brief Compute geometry normal.
    \param p1 First point.
    \param p2 Second point.
    \param p3 Third point.
    \return Geometry normal.

    \rst
    Note that the three points must be given in counter-clockwised order.
    \endrst
*/
static Vec3 geometry_normal(Vec3 p1, Vec3 p2, Vec3 p3) {
    return glm::normalize(glm::cross(p2 - p1, p3 - p1));
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
static VecT mix_barycentric(VecT a, VecT b, VecT c, Vec2 uv) {
    return a * (1_f - uv.x - uv.y) + b * uv.x + c * uv.y;
}

/*!
    \brief Convert spherical to cartesian coordinates.
*/
static Vec3 spherical_to_cartesian(Float theta, Float phi) {
    const auto sinTheta = glm::sin(theta);
    return Vec3(
        sinTheta * glm::cos(phi),
        sinTheta * glm::sin(phi),
        glm::cos(theta)
    );
}

/*!
    \brief Compute sin in local shading coordinates.
*/
static Float local_sin(Vec3 local_d) {
    return safe_sqrt(1_f - local_d.z * local_d.z);
}

/*!
    \brief Compute cos in local shading coordinates.
*/
static Float local_cos(Vec3 local_d) {
    return local_d.z;
}

/*!
    \brief Compute tan in local shading coordinates.
*/
static Float local_tan(Vec3 local_d) {
    const auto t = 1_f - local_d.z * local_d.z;
    return t <= 0_f ? 0_f : std::sqrt(t) / local_d.z;
}

/*!
    \brief Compute tan^2 in local shading coordinates.
*/
static Float local_tan2(Vec3 local_d) {
    if (local_d.z == 0_f) {
        return Inf;
    }
    const auto cos2 = local_d.z * local_d.z;
    const auto sin2 = 1_f - cos2;
    return sin2 <= 0_f ? 0_f : sin2 / cos2;
}

#pragma endregion

#pragma region Sampling related

/*!
    \brief Uniform sampling on unit disk.
    \param u Random variable in [0,1]^2.
    \return Sampled value.
*/
static Vec2 sample_uniform_disk(Vec2 u) {
    const auto r = safe_sqrt(u[0]);
    const auto t = 2_f * Pi * u[1];
    const auto x = r * std::cos(t);
    const auto y = r * std::sin(t);
    return { x, y };
}

/*!
    \brief PDF of uniform distribution on unit disk.
    \return Evaluated PDF.
*/
static constexpr Float pdf_uniform_disk() {
    return 1_f / Pi;
}

/*!
    \brief Cosine-weighted direction sampling.
    \param u Random variable in [0,1]^2.
    \return Sampled value.
*/
static Vec3 sample_cosine_weighted(Vec2 u) {
    const auto r = safe_sqrt(u[0]);
    const auto t = 2_f * Pi * u[1];
    const auto x = r * std::cos(t);
    const auto y = r * std::sin(t);
    return { x, y, safe_sqrt(1_f - x*x - y*y) };
}

/*!
    \brief Evauate PDF of cosine-weighted distribution on a sphere in projected solid angle measure.
    \return Evaluated PDF.
*/
static constexpr Float pdf_cosine_weighted_projSA() {
    return 1_f / Pi;
}

/*!
    \brief Uniformly sample a direction from a sphere.
    \param u Random variable in [0,1]^2.
    \return Sampled value.
*/
static Vec3 sample_uniform_sphere(Vec2 u) {
    const auto z = 1_f - 2_f * u[0];
    const auto r = safe_sqrt(1_f - z*z);
    const auto t = 2_f * Pi * u[1];
    const auto x = r * std::cos(t);
    const auto y = r * std::sin(t);
    return { x, y, z };
}

/*!
    \brief PDF of uniform direction on a sphere in solid angle measure.
    \return Evaluated density.
*/
static constexpr Float pdf_uniform_sphere() {
    return 1_f / (4_f * Pi);
}

/*!
    \brief Compute balance heuristics.
    \param p1 Evalauted PDF for the first strategy.
    \param p2 Evaluated PDF for the second strategy.
    \return Evaluated weight.
*/
static Float balance_heuristic(Float p1, Float p2) {
    if (p1 == 0_f && p2 == 0_f) {
        return 0_f;
    }
    return p1 / (p1 + p2);
}

#pragma endregion

/*!
    @}
*/

LM_NAMESPACE_END(math)

// ------------------------------------------------------------------------------------------------

/*!
    \addtogroup math
    @{
*/

#pragma region Transform

/*!
    \brief Transform.
*/
struct Transform {
    Mat4 M;             //!< Transform associated to the primitive
    Mat3 normal_M;      //!< Transform for normals
    Float J;            //!< J := |det(M_lin)| where M_lin is linear component of M

    //! \cond
    template <typename Archive>
    void serialize(Archive& ar) {
        ar(M, normal_M, J);
    }
    //! \endcond

    Transform() = default;

    /*!
        \brief Construct the transform with 4x4 transformation matrix.
        \param M Transformation matrix.
    */
    Transform(const Mat4& M) : M(M) {
        normal_M = Mat3(glm::transpose(glm::inverse(M)));
        J = std::abs(glm::determinant(Mat3(M)));
    }
};

#pragma endregion

/*!
    @}
*/

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_END(LM_NAMESPACE)
