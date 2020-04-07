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
    Please refer to :ref:`path_sampling` for detail.
    \endrst
*/
class Light : public Component {
public:
    /*!
        \brief Set scene bound.
        \param bound Scene bound.
    */
    virtual void set_scene_bound(const Bound& bound) {
        LM_UNUSED(bound);
    }

    // --------------------------------------------------------------------------------------------

    //! Result of primary ray sampling.
    struct RaySample {
        PointGeometry geom;   //!< Sampled geometry information.
        Vec3 wo;              //!< Sampled direction.
        Vec3 weight;          //!< Contribution divided by probability.
    };

    //! Random number input for primary ray sampling.
    struct RaySampleU {
        Vec2 up;
        Float upc;
        Vec2 ud;
    };

    /*!
        \brief Primary ray sampling.
        \param u Random number input.
        \param transform Transformation of the light source.

        \rst
        This function samples a primary ray generated from the light
        :math:`(\mathbf{x}, \omega) \sim p_{\mu^* L}(\cdot,\cdot)`.
        \endrst
    */
    virtual std::optional<RaySample> sample_ray(const RaySampleU& u, const Transform& transform) const = 0;

    /*!
        \brief Evaluate PDF for primary ray sampling.
        \param geom Sampled point geometry.
        \param wo Sampled outgoing direction.
        \param transform Transformation of the light source.
        \param eval_delta If true, evaluate delta function.

        \rst
        The function evaluates the PDF :math:`p_{\mu^* L}(\mathbf{x}, \omega)`.
        \endrst
    */
    virtual Float pdf_ray(const PointGeometry& geom, Vec3 wo, const Transform& transform, bool eval_delta) const = 0;

    // --------------------------------------------------------------------------------------------

    //! Result of direction sampling.
    struct DirectionSample {
        Vec3 wo;
        Vec3 weight;
    };

    //! Random number input for direction sampling.
    struct DirectionSampleU {
        Vec2 ud;
    };

    /*!
        \brief Direction sampling.
        \param u Random number input.
        \param geom Point geometry of the point on the sensor.

        \rst
        This function sample a direction from the point on the sensor:
        :math:`\omega \sim p_{\sigma^* L}(\cdot\mid\mathbf{x})`.
        \endrst
    */
    virtual std::optional<DirectionSample> sample_direction(const DirectionSampleU& u, const PointGeometry& geom) const = 0;

    /*!
        \brief Evaluate PDF for direction sampling.
        \param geom Point geometry of the point on the sensor.
        \param wo Sampled outgoing direction.

        \rst
        This function evaluats the PDF :math:`p_{\sigma^* L}(\omega\mid\mathbf{x})`.
        \endrst
    */
    virtual Float pdf_direction(const PointGeometry& geom, Vec3 wo) const = 0;

    // --------------------------------------------------------------------------------------------

    //! Result of endpint sampling.
    struct PositionSample {
        PointGeometry geom;
        Vec3 weight;
    };

    //! Random number input for endpoint sampling.
    struct PositionSampleU {
        Vec2 up;
        Float upc;
    };

    /*!
        \brief Endpoint sampling.
        \param u Random number input.
        \param transform Transformation of the light source.

        \rst
        This function samples a point on the sensor
        :math:`\mathbf{x} \sim p_{AL}(\cdot)`.
        \endrst
    */
    virtual std::optional<PositionSample> sample_position(const PositionSampleU& u, const Transform& transform) const = 0;

    /*!
        \brief Evaluate PDF for endpoint sampling.
        \param geom Sampled point geometry.
        \param transform Transformation of the light source.

        \rst
        This function evaluates the PDF :math:`p_{AL}(\mathbf{x})`.
        \endrst
    */
    virtual Float pdf_position(const PointGeometry& geom, const Transform& transform) const = 0;

    // --------------------------------------------------------------------------------------------

    /*!
        \brief Direct endpoint sampling.
        \param u Random number input.
        \param geom Point geometry on the scene surface.
        \param transform Transformation of the light source.

        \rst
        This function samples a direction from the surface point toward the point on the light source:
        :math:`\omega \sim p_{\sigma^* \mathrm{directL}}(\cdot\mid\mathbf{x})`.
        The transformation of the light source is specified by ``transform``.
        For convenience, the function also returns the point on the light source
        in the direction of the ray :math:`(\mathbf{x}',\omega)`.
        \endrst
    */
    virtual std::optional<RaySample> sample_direct(const RaySampleU& u, const PointGeometry& geom, const Transform& transform) const = 0;

    /*!
        \brief Evaluate pdf for direct endpoint sampling.
        \param geom Point geometry on the scene surface.
        \param geomL Point geometry on the light source.
        \param transform Transformation of the light source.
        \param wo Outgoing direction from the point of the light source.
        \param eval_delta If true, evaluate delta function.

        \rst
        This function evalutes the PDF :math:`p_{\sigma^* \mathrm{directL}}(\omega\mid\mathbf{x})`.
        \endrst
    */
    virtual Float pdf_direct(const PointGeometry& geom, const PointGeometry& geomL, const Transform& transform, Vec3 wo, bool eval_delta) const = 0;

    // --------------------------------------------------------------------------------------------

    //! Check if the light contains delta component.
    virtual bool is_specular() const = 0;

    //! Check if the light source is inifite distant light.
    virtual bool is_infinite() const = 0;

    //! Check if the light source is environment light.
    virtual bool is_env() const { return false; }

    //! Check if the endpoint is connectable.
    virtual bool is_connectable(const PointGeometry& geom) const = 0;

    /*!
        \brief Evaluate luminance.
        \param geom Point geometry on the light source.
        \param wo Outgoing direction from the point of the light source.
        \param eval_delta If true, evaluate delta function.

        \rst
        This function evaluates luminance function :math:`L_e`.
        \endrst
    */
    virtual Vec3 eval(const PointGeometry& geom, Vec3 wo, bool eval_delta) const = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
