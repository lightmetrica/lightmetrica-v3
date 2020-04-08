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
    \addtogroup camera
    @{
*/

/*!
    \brief Camera.

    \rst
    This component interface represents a camera inside the scene,
    responsible for sampling and evaluation of the rays emitted from/to the camera. 
    Please refer to :ref:`path_sampling` for detail.
    \endrst
*/
class Camera : public Component {
public:
    /*!
        \brief Set aspect ratio.
        \param aspect Aspect ratio (width / height).

        \rst
        This function sets the aspect ratio of the film to be rendered.
        Note that this function overrides the aspect ratio given by a parameter of
        :cpp:func:`lm::Component::construct` function.
        \endrst
    */
    virtual void set_aspect_ratio(Float aspect) = 0;

    // --------------------------------------------------------------------------------------------

    /*!
        \brief Get view matrix if available.
        \return View matrix.
    */
    virtual Mat4 view_matrix() const = 0;

    /*!
        \brief Get projection matrix if available.
        \return Projection matrix.
    */
    virtual Mat4 projection_matrix() const = 0;

    // --------------------------------------------------------------------------------------------

    /*!
        \brief Generate a primary ray with the corresponding raster position.
        \param rp Raster position.

        \rst
        This function deterministically generates a ray from the given raster position in :math:`[0,1]^2`
        corresponding to width and height of the screen, leaf-to-right and bottom-to-top.
        This function is useful in the application that the primary ray is fixed (e.g., ray casting).
        Use :cpp:func:`sample_ray` when the primary ray is generated randomly.
        \endrst
    */
    virtual Ray primary_ray(Vec2 rp) const = 0;

    //! Result of primary ray sampling.
    struct RaySample {
        PointGeometry geom;     //!< Sampled point geometry.
        Vec3 wo;                //!< Sampled direction.
        Vec3 weight;            //!< Contribution divided by probability.
    };

    //! Random number input for primary ray sampling.
    struct RaySampleU {
        Vec2 ud;
    };

    /*!
        \brief Primary ray sampling.
        \param u Random number input.

        \rst
        This function samples a primary ray generated from the sensor
        :math:`(\mathbf{x}, \omega) \sim p_{\mu^* E}(\cdot,\cdot)`.
        \endrst
    */
    virtual std::optional<RaySample> sample_ray(const RaySampleU& u) const = 0;

    /*!
        \brief Evaluate PDF for primary ray sampling.
        \param geom Sampled point geometry.
        \param wo Sampled outgoing direction.
        
        \rst
        This function evaluate the PDF :math:`p_{\mu^* E}(\mathbf{x}, \omega)`.
        \endrst
    */
    virtual Float pdf_ray(const PointGeometry& geom, Vec3 wo) const = 0;

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
        :math:`\omega \sim p_{\sigma^* E}(\cdot\mid\mathbf{x})`.
        \endrst
    */
    virtual std::optional<DirectionSample> sample_direction(const DirectionSampleU& u, const PointGeometry& geom) const = 0;

    /*!
        \brief Evaluate PDF for direction sampling.
        \param geom Point geometry of the point on the sensor.
        \param wo Sampled outgoing direction.

        \rst
        This function evaluats the PDF :math:`p_{\sigma^* E}(\omega\mid\mathbf{x})`.
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
        Vec2 udp;
    };

    /*!
        \brief Endpoint sampling.
        \param u Random number input.

        \rst
        This function samples a point on the sensor
        :math:`\mathbf{x} \sim p_{AE}(\cdot)`.
        \endrst
    */
    virtual std::optional<PositionSample> sample_position(const PositionSampleU& u) const = 0;

    /*!
        \brief Evaluate PDF for endpoint sampling.
        \param geom Sampled point geometry.

        \rst
        This function evaluates the PDF :math:`p_{AE}(\mathbf{x})`.
        \endrst
    */
    virtual Float pdf_position(const PointGeometry& geom) const = 0;

    // --------------------------------------------------------------------------------------------

    /*!
        \brief Direct endpoint sampling.
        \param u Random number input.
        \param geom Point geometry.

        \rst
        This function sample a direction from the sensor given a point on the surface
        :math:`\omega \sim p_{\sigma^* \mathrm{directE}}(\cdot\mid\mathbf{x})`.
        \endrst
    */
    virtual std::optional<RaySample> sample_direct(const RaySampleU& u, const PointGeometry& geom) const = 0;

    /*!
        \brief Evaluate PDF for direct endpoint sampling.
        \param geom Point on the surface.
        \param geomE Sampled point on the sensor.
        \param wo Sampled direction.

        \rst
        This function evaluates the PDF :math:`p_{\sigma^* \mathrm{directE}}(\omega\mid\mathbf{x})`.
        \endrst
    */
    virtual Float pdf_direct(const PointGeometry& geom, const PointGeometry& geomE, Vec3 wo) const = 0;

    // --------------------------------------------------------------------------------------------

    /*!
        \brief Compute a raster position.
        \param wo Primary ray direction.
        \return Raster position.
    */
    virtual std::optional<Vec2> raster_position(Vec3 wo) const = 0;

    /*!
        \brief Check if the sensor is connectable.

        \rst
        This function checks if the sensor is connectable with other scene surface.
        For detail, please refer to :ref:`path_sampling_connectable_endpoint`.
        \endrst
    */
    virtual bool is_connectable(const PointGeometry& geom) const = 0;

    /*!
        \brief Evaluate sensitivity.
        \param wo Outgoing direction.

        \rst
        Evaluates sensitivity function :math:`W_e(\mathbf{x},\omega_o)` of the sensor.
        \endrst
    */
    virtual Vec3 eval(Vec3 wo) const = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
