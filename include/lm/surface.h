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

    \rst
    This structure represents a point inside the scene,
    which includes a surface point, point in the air, or point at infinity, etc.
    This structure is a basic quantity to be used in various renderer-related functions,
    like sampling or evaluation of terms.
    The structure represents three types of points.

    (1) *A point on a scene surface*.
        The structure describes a point on the scene surface if ``degenerated=false``.
        You can access the following associated information.

        - Position ``p``
        - Shading normal ``n``
        - Texture coordinates ``t``
        - Tangent vectors ``u`` and ``v``

    (2) *A point in a media*.
        The structure describes a point in a media if ``degenerated=true``,
        for instance used to represent a position of point light or pinhole camera.
        The associated information is the position ``p``.

    (3) *A point at infinity*.
        The structure describes a point at infinity if ``infinite=true``,
        used to represent a point generated by directional light or environment light.
        This point does not represent an actual point associated with certain position
        in the 3d space, but it is instead used to simplify the renderer interfaces.
        The associated information is the direction from the point at infinity ``wo``.
    \endrst
*/
struct PointGeometry {
    bool degenerated;       //!< True if surface is degenerated (e.g., point light).
    bool infinite;          //!< True if the point is a point at infinity.
    Vec3 p;                 //!< Position.
    Vec3 n;                 //!< Shading normal.
    Vec3 gn;                //!< Geometry normal.
    Vec3 wo;                //!< Direction from a point at infinity (used only when infinite=true).
    Vec2 t;                 //!< Texture coordinates.
    Vec3 u, v;              //!< Orthogonal tangent vectors.
    Mat3 to_world;          //!< Matrix to convert to world coordinates.
    Mat3 to_local;          //!< Matrix to convert to local shading coordinates.

    /*!
        \brief Make degenerated point.
        \param p Position.

        \rst
        A static function to generate a point in the air
        from the specified position ``p``.
        \endrst
    */
    static PointGeometry make_degenerated(Vec3 p) {
        PointGeometry geom;
        geom.degenerated = true;
        geom.infinite = false;
        geom.p = p;
        return geom;
    }

    /*!
        \brief Make a point at infinity.
        \param wo Direction from a point at infinity.

        \rst
        A static function to generate a point at infinity
        from the specified direction from the point.
        \endrst
    */
    static PointGeometry make_infinite(Vec3 wo) {
        PointGeometry geom;
        geom.degenerated = false;
        geom.infinite = true;
        geom.wo = wo;
        return geom;
    }

    /*!
        \brief Make a point at infinity.
        \param wo Direction from a point at infinity.
        \param p Rrepresentative distant point (e.g., a point outside the scene bound).
    */
    static PointGeometry make_infinite(Vec3 wo, Vec3 p) {
        PointGeometry geom;
        geom.degenerated = false;
        geom.infinite = true;
        geom.wo = wo;
        geom.p = p;
        return geom;
    }

    /*!
        \brief Make a point on surface.
        \param p Position.
        \param n Shading normal.
        \param gn Geometry normal.
        \param t Texture coordinates.

        \rst
        A static function to generate a point on the scene surface
        from the specified surface geometry information.
        \endrst
    */
    static PointGeometry make_on_surface(Vec3 p, Vec3 n, Vec3 gn, Vec2 t) {
        PointGeometry geom;
        geom.degenerated = false;
        geom.infinite = false;
        geom.p = p;
        geom.n = n;
        geom.gn = gn;
        geom.t = t;
        std::tie(geom.u, geom.v) = math::orthonormal_basis(n);
        geom.to_world = Mat3(geom.u, geom.v, geom.n);
        geom.to_local = glm::transpose(geom.to_world);
        return geom;
    }

    /*!
        \brief Make a point on surface.
        \param p Position.
        \param n Shading normal.
        \param gn Geometry normal.

        \rst
        A static function to generate a point on the scene surface
        from the specified surface geometry information.
        \endrst
    */
    static PointGeometry make_on_surface(Vec3 p, Vec3 n, Vec3 gn) {
        return make_on_surface(p, n, gn, {});
    }

    /*!
        \brief Checks if two directions lies in the same half-space.
        \param w1 First direction.
        \param w2 Second direction.

        \rst
        This function checks if two directions ``w1`` and ``w2``
        lies in the same half-space divided by the tangent plane.
        ``w1`` and ``w2`` are interchangeable.
        \endrst
    */
    bool opposite(Vec3 w1, Vec3 w2) const {
        return glm::dot(w1, n) * glm::dot(w2, n) <= 0;
    }

    /*!
        \brief Compute orthonormal basis according to the incident direction.
        \param wi Incident direction.

        \rst
        The function returns an orthornormal basis according to the incident direction ``wi``.
        If ``wi`` is coming below the surface, the orthonormal basis are created
        based on the negated normal vector. This function is useful to support two-sided materials.
        \endrst
    */
    std::tuple<Vec3, Vec3, Vec3> orthonormal_basis_twosided(Vec3 wi) const {
        const int i = glm::dot(wi, n) > 0;
        return { i ? n : -n, u, i ? v : -v };
    }
};

/*!
    \brief Scene interaction.

    \rst
    This structure represents a point of interaction between a light and the scene.
    The point represents a scattering point of an endpoint of a light transportation path,
    defined either on a surface or in a media.
    The point is associated with a geometry information around the point
    and an index of the primitive.
    The structure is used by the functions of :cpp:class:`lm::Scene` class.
    Also, the structure can represent special terminator
    representing the sentinels of the light path, which can be used to provide
    conditions to determine subspace of endpoint.
    \endrst
*/
struct SceneInteraction {
    // Scene interaction type.
    enum Type {
        None               = 0,
        CameraEndpoint     = 1<<0,
        LightEndpoint      = 1<<1,
        SurfaceInteraction = 1<<2,
        MediumInteraction  = 1<<3,
        Endpoint           = CameraEndpoint | LightEndpoint,
        Midpoint           = SurfaceInteraction | MediumInteraction
    };

    int type = None;        //!< Scene interaction type.
    int primitive;          //!< Primitive node index.
    PointGeometry geom;     //!< Surface point geometry information.

    /*!
        \brief Check the scene interaction type.
    */
    bool is_type(int type_flag) const {
        return (type & type_flag) > 0;
    }

    /*!
        \brief Consider the scene interaction as a different type.
    */
    SceneInteraction as_type(int new_type) const {
        auto sp = *this;
        sp.type = new_type;
        return sp;
    };

    /*!
        \brief Make surface interaction.
        \param primitive Primitive index.
        \param geom Point geometry.
        \return Created scene interaction.
    */
    static SceneInteraction make_surface_interaction(int primitive, const PointGeometry& geom) {
        SceneInteraction sp;
        sp.type = SurfaceInteraction;
        sp.primitive = primitive;
        sp.geom = geom;
        return sp;
    }

    /*!
        \brief Make medium interaction.
        \param primitive Primitive index.
        \param geom Point geometry.
        \return Created scene interaction.
    */
    static SceneInteraction make_medium_interaction(int primitive, const PointGeometry& geom) {
        SceneInteraction sp;
        sp.type = MediumInteraction;
        sp.primitive = primitive;
        sp.geom = geom;
        return sp;
    }

    /*!
        \brief Make camera endpoint.
        \param primitive Primitive index.
        \param geom Point geometry.
        \param window Window in raster coordinates.
        \param aspect Aspect ratio.
        \return Created scene interaction.
    */
    static SceneInteraction make_camera_endpoint(int primitive, const PointGeometry& geom) {
        SceneInteraction sp;
        sp.type = CameraEndpoint;
        sp.primitive = primitive;
        sp.geom = geom;
        return sp;
    }

    /*!
        \brief Make light endpoint.
        \param primitive Primiive index.
        \param geom Point geometry.
    */
    static SceneInteraction make_light_endpoint(int primitive, const PointGeometry& geom) {
        SceneInteraction sp;
        sp.type = LightEndpoint;
        sp.primitive = primitive;
        sp.geom = geom;
        return sp;
    }
};

/*!
    \brief Light transport direction.
*/
enum class TransDir {
    LE,     //!< Transport direction is L (light) to E (sensor).
    EL      //!< Transport direction is E (sensor) to L (light).
};

/*!
    @}
*/

LM_NAMESPACE_BEGIN(surface)

/*!
    \addtogroup scene
    @{
*/

/*!
    \brief Compute geometry term.
    \param s1 First point.
    \param s2 Second point.

    \rst
    This function computes the geometry term :math:`G(\mathbf{x}\leftrightarrow\mathbf{y})`
    from the two surface points :math:`\mathbf{x}` and :math:`\mathbf{y}`.
    This function can accepts extended points represented by :cpp:class:`lm::PointGeometry`.
    \endrst
*/
static Float geometry_term(const PointGeometry& s1, const PointGeometry& s2) {
    assert(!(s1.infinite && s2.infinite));
    Vec3 d;
    Float L2;
    if (s1.infinite || s2.infinite) {
        d = s1.infinite ? s1.wo : -s2.wo;
        L2 = 1_f;
    }
    else {
        d = s2.p - s1.p;
        L2 = glm::dot(d, d);
        d = d / std::sqrt(L2);
    }
    const auto cos1 = s1.degenerated || s1.infinite ? 1_f : glm::abs(glm::dot(s1.n, d));
    const auto cos2 = s2.degenerated || s2.infinite ? 1_f : glm::abs(glm::dot(s2.n, -d));
    return cos1 * cos2 / L2;
}

/*!
    \brief Copmute distance between two points.
    \param s1 First point.
    \param s2 Second point.

    \rst
    This function computes the distance between two points.
    If one of the points is a point at infinity, returns inf.
    \endrst
*/
static Float distance(const PointGeometry& s1, const PointGeometry& s2) {
    return s1.infinite || s2.infinite ? Inf : glm::distance(s1.p, s2.p);
}

/*!
    \brief Convert PDF in solid angle measure to projected solid angle measure.

    \rst
    If the point geometry is not degenerated, this function converts the given pdf to
    the projected solid angle measure. If the point geometry is degenerated,
    this function keeps the solid angle measure.
    \endrst
*/
static Float convert_pdf_SA_to_projSA(Float pdf_SA, const PointGeometry& geom, Vec3 d) {
    if (geom.degenerated) {
        // When geom is degenerated, use pdf with solid angle measure.
        return pdf_SA;
    }
    const auto J = glm::abs(glm::dot(geom.n, d));
    if (J == 0_f) {
        // When normal and outgoing direction are perpecdicular,
        // we can consider pdf is always zero because
        // in this case the contriubtion function becomes always zero.
        return 0_f;
    }
    return pdf_SA / J;
}

/*!
    \brief Convert PDF in projected solid angle measure to area measure.
*/
static Float convert_pdf_projSA_to_area(Float pdf_projSA, const PointGeometry& geom1, const PointGeometry& geom2) {
    const auto G = geometry_term(geom1, geom2);
    return pdf_projSA * G;
}

/*!
    \brief Energy compensation factor for shading normal.
*/
static Float shading_normal_correction(const PointGeometry& geom, Vec3 wi, Vec3 wo, TransDir trans_dir) {
    const auto local_wi = geom.to_local * wi;
    const auto local_wo = geom.to_local * wo;
    const auto wi_dot_ng = glm::dot(wi, geom.gn);
    const auto wo_dot_ng = glm::dot(wo, geom.gn);
    const auto wi_dot_ns = math::local_cos(local_wi);
    const auto wo_dot_ns = math::local_cos(local_wo);
    if (wi_dot_ng * wi_dot_ns <= 0_f || wo_dot_ng * wo_dot_ns <= 0_f) {
        return 0_f;
    }
    if (trans_dir == TransDir::LE) {
        return wi_dot_ns * wo_dot_ng / (wo_dot_ns * wi_dot_ng);
    }
    return 1_f;
}

/*!
    @}
*/

LM_NAMESPACE_END(surface)
LM_NAMESPACE_END(LM_NAMESPACE)
