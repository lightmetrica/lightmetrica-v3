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
    \endrst
*/
class Camera : public Component {
public:
    /*!
        \brief Set aspect ratio.

        \rst
        This function sets the aspect ratio of the film to be rendered.
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
        \param aspect Aspect ratio of the film.
        \return Projection matrix.
    */
    virtual Mat4 projection_matrix() const = 0;

    // --------------------------------------------------------------------------------------------

    /*!
        \brief Generate a primary ray with the corresponding raster position.
        \param rp Raster position.
        \param aspect Aspect ratio of the film.

        \rst
        This function deterministically generates a ray from the given raster position in :math:`[0,1]^2`
        corresponding to width and height of the screen, leaf-to-right and bottom-to-top.
        This function is useful in the application that the primary ray is fixed (e.g., ray casting).
        Use :cpp:func:`samplePrimaryRay` when the primary ray is generated randomly.
        \endrst
    */
    virtual Ray primary_ray(Vec2 rp) const = 0;

    /*!
        \brief Result of primary ray sampling.
    
        \rst
        This structure represents the result of
        :cpp:func:`lm::Camera::sample_primary_ray` function.
        \endrst
    */
    struct RaySample {
        PointGeometry geom;     //!< Sampled geometry information.
        Vec3 wo;                //!< Sampled direction.
        Vec3 weight;            //!< Contribution divided by probability.
    };

    /*!
    */
    struct RaySampleU {
        Vec2 ud;
    };

    /*!
        \brief Sample a primary ray within the given raster window.
        \param rng Random number generator.
        \param window Raster window.
        \param aspect Aspect ratio of the film.

        \rst
        This function generates a uniformly random ray from the raster window inside the screen.
        The raster windows is specified by subregion of :math:`[0,1]^2` 
        using 4d vector containing ``(x,y,w,h)`` where ``(x,y)`` is the bottom-left point
        of the region, and ``(w,h)`` is width and height of the region.

        The sampled results are returned by :cpp:func:`CameraRaySample` structure.
        When sampling has failed, the function returns nullopt.
        
        Mathematically the function defines a sampling from the distribution
        having a joint probability density :math:`p(\mathbf{x}, \omega)` defined by
        the surface point :math:`\mathbf{x}` and the direction :math:`\omega`
        according to the produce measure of area measure and projected solid angle measure.

        Note that the generated rays are uniform in a sense that
        each ray is generated from the uniform sampling of the raster window.
        Looking by the solid angle measure, for instance, the set of rays are not uniform.
        \endrst
    */
    virtual std::optional<RaySample> sample_ray(const RaySampleU& u) const = 0;

    /*!
    */
    virtual Float pdf_ray(const PointGeometry& geom, Vec3 wo) const = 0;

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
    virtual std::optional<DirectionSample> sample_direction(const DirectionSampleU& u) const = 0;

    /*!
        \brief Evaluate pdf for direction sampling.
        \param wo Outgoing direction.
        \param aspect Aspect ratio of the film.
        \return Evaluated pdf.
    */
    virtual Float pdf_direction(Vec3 wo) const = 0;

    // --------------------------------------------------------------------------------------------

    /*!
    */
    struct PositionSample {
        PointGeometry geom;
        Vec3 weight;
    };

    /*!
    */
    virtual std::optional<PositionSample> sample_position() const = 0;

    /*!
    */
    virtual Float pdf_position(const PointGeometry& geom) const = 0;

    // --------------------------------------------------------------------------------------------

    /*!
    */
    virtual std::optional<RaySample> sample_direct(const RaySampleU& u, const PointGeometry& geom) const = 0;

    /*!
    */
    virtual Float pdf_direct(const PointGeometry& geom, const PointGeometry& geomE, Vec3 wo) const = 0;

    // --------------------------------------------------------------------------------------------

    /*!
        \brief Compute a raster position.
        \param wo Primary ray direction.
        \param aspect Aspect ratio of the film.
        \return Raster position.
    */
    virtual std::optional<Vec2> raster_position(Vec3 wo) const = 0;

    /*!
    */
    virtual bool is_connectable(const PointGeometry& geom) const = 0;

    /*!
        \brief Evaluate sensitivity.
        \param wo Outgoing direction.
        \param aspect Aspect ratio of the film.

        \rst
        Evaluates sensitivity function :math:`W_e(\mathbf{x}, \omega)` of the camera.
        \endrst
    */
    virtual Vec3 eval(Vec3 wo) const = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
