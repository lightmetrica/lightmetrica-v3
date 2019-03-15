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
    \brief Result of primary ray sampling.
    
    \rst
    This structure represents the result of
    :cpp:func:`lm::Camera::samplePrimaryRay` function.
    \endrst
*/
struct CameraRaySample {
    PointGeometry geom;   //!< Sampled geometry information.
    Vec3 wo;              //!< Sampled direction.
    Vec3 weight;          //!< Contribution divided by probability.
};

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
        \brief Check if the camera contains delta component.
        \param geom Surface geometry information.

        \rst
        This function returns true if the sensitivity function :math:`W_e` of the camera
        contains a component of delta function.
        \endrst
    */
    virtual bool isSpecular(const PointGeometry& geom) const {
        LM_UNUSED(geom);
        LM_UNREACHABLE_RETURN();
    }

    /*!
        \brief Generate a primary ray with the corresponding raster position.
        \param rp Raster position.
        \param aspectRatio Aspect ratio of the film.

        \rst
        This function deterministically generates a ray from the given raster position in :math:`[0,1]^2`
        corresponding to width and height of the screen, leaf-to-right and bottom-to-top.
        This function is useful in the application that the primary ray is fixed (e.g., ray casting).
        Use :cpp:func:`samplePrimaryRay` when the primary ray is generated randomly.
        \endrst
    */
    virtual Ray primaryRay(Vec2 rp, Float aspectRatio) const {
        LM_UNUSED(rp, aspectRatio);
        LM_UNREACHABLE_RETURN();
    }

    /*!
        \brief Sample a primary ray within the given raster window.
        \param rng Random number generator.
        \param window Raster window.
        \param aspectRatio Aspect ratio of the film.

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
    virtual std::optional<CameraRaySample> samplePrimaryRay(Rng& rng, Vec4 window, Float aspectRatio) const {
        LM_UNUSED(rng, window, aspectRatio);
        LM_UNREACHABLE_RETURN();
    }

    /*!
        \brief Evaluate sensitivity.
        \param geom Surface geometry information.
        \param wo Outgoing direction.

        \rst
        Evaluates sensitivity function :math:`W_e(\mathbf{x}, \omega)` of the camera.
        \endrst
    */
    virtual Vec3 eval(const PointGeometry& geom, Vec3 wo) const {
        LM_UNUSED(geom, wo);
        LM_UNREACHABLE_RETURN();
    }

    /*!
        \brief Get view matrix if available.
        \return View matrix.
    */
    virtual Mat4 viewMatrix() const {
        LM_UNREACHABLE_RETURN();
    }

    /*!
        \brief Get projection matrix if available.
        \return Projection matrix.
    */
    virtual Mat4 projectionMatrix() const {
        LM_UNREACHABLE_RETURN();
    }
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
