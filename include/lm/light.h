/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "math.h"
#include "surface.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup light
    @{
*/

/*!
    \brief Light.

    \rst
    This component interface represents a light source inside the scene.
    responsible for sampling and evaluation of the rays emitted from/to the light source. 
    \endrst
*/
class Light : public Component {
public:
    /*!
        \brief Set scene bound.
    */
    virtual void set_scene_bound(const Bound& bound) {
        LM_UNUSED(bound);
    }

    // --------------------------------------------------------------------------------------------

    /*!
        \brief Result of light sampling.

        \rst
        This structure represents the result of
        :cpp:func:`lm::Light::sample` function.
        \endrst
    */
    struct RaySample {
        PointGeometry geom;   //!< Sampled geometry information.
        Vec3 wo;              //!< Sampled direction.
        Vec3 weight;          //!< Contribution divided by probability.
    };

    /*!
    */
    struct RaySampleU {
        Vec2 up;
        Float upc;
        Vec2 ud;
    };

    /*!
    */
    virtual std::optional<RaySample> sample_ray(const RaySampleU& u, const Transform& transform) const = 0;

    /*!
    */
    virtual Float pdf_ray(const PointGeometry& geom, Vec3 wo, const Transform& transform) const = 0;

    // --------------------------------------------------------------------------------------------

    /*!
    */
    struct DirectionSample {
        Vec3 wo;
        Vec3 weight;
    };

    /*!
    */
    struct DirectionSampleU {
        Vec2 ud;
    };

    /*!
    */
    virtual std::optional<DirectionSample> sample_direction(const PointGeometry& geom, const DirectionSampleU& u) const = 0;

    /*!
    */
    virtual Float pdf_direction(const PointGeometry& geom, Vec3 wo) const = 0;

    // --------------------------------------------------------------------------------------------

    /*!
    */
    struct PositionSample {
        PointGeometry geom;
        Vec3 weight;
    };

    /*!
    */
    struct PositionSampleU {
        Vec2 up;
        Float upc;
    };

    /*!
    */
    virtual std::optional<PositionSample> sample_position(const PositionSampleU& u, const Transform& transform) const = 0;

    /*!
    */
    virtual Float pdf_position(const PointGeometry& geom, const Transform& transform) const = 0;

    // --------------------------------------------------------------------------------------------

    /*!
        \brief Sample a position on the light.
        \param rng Random number generator.
        \param geom Point geometry on the scene surface.
        \param transform Transformation of the light source.

        \rst
        This function samples a direction from the surface point ``geom``
        toward the point on the light source.
        The transformation of the light source is specified by ``transform``.
        
        Mathmatically, the function samples a direction :math:`\omega`
        from the conditional probability density :math:`p(\omega\mid \mathbf{x}')`
        in projected solid angle measure given the surface point :math:`\mathbf{x}'`.

        For convenience, the function also returns the point on the light source
        in the direction of the ray :math:`(\mathbf{x}',\omega)`.
        If there is no corresponding surface geometry in the case of distant light source,
        ``geom.infinite`` becomes true.
        \endrst
    */
    virtual std::optional<RaySample> sample_direct(const RaySampleU& u, const PointGeometry& geom, const Transform& transform) const = 0;

    /*!
        \brief Evaluate pdf for light sampling in projected solid angle measure.
        \param geom Point geometry on the scene surface.
        \param geomL Point geometry on the light source.
        \param comp Component index.
        \param transform Transformation of the light source.
        \param wo Outgoing direction from the point of the light source.

        \rst
        This function evalutes a pdf corresponding to :cpp:func:`lm::Light::sample` function.
        ``geomL`` is either the point on the light source or inifite point.
        If the given direction cannot be sampled, the function returns zero.
        \endrst
    */
    virtual Float pdf_direct(const PointGeometry& geom, const PointGeometry& geomL, const Transform& transform, Vec3 wo) const = 0;

    // --------------------------------------------------------------------------------------------

    /*!
        \brief Check if the light source is inifite distant light.
        \rst
        This function checks if the light source is inifite distant light
        such as environment light or directional light.
        \endrst
    */
    virtual bool is_infinite() const = 0;

    /*!
    */
    virtual bool is_connectable(const PointGeometry& geom) const = 0;

    /*!
        \brief Evaluate luminance.
        \param geom Point geometry on the light source.
        \param comp Component index.
        \param wo Outgoing direction from the point of the light source.

        \rst
        This function evaluates luminance function :math:`L_e` of the light source.
        \endrst
    */
    virtual Vec3 eval(const PointGeometry& geom, Vec3 wo) const = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
