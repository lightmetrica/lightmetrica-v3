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
    \brief Result of Camera::samplePrimaryRay() function.
*/
struct CameraRaySample {
    PointGeometry geom;   // Sampled geometry information
    Vec3 wo;              // Sampled direction
    Vec3 weight;          // Contribution divided by probability
};

/*!
    \brief Camera.
*/
class Camera : public Component {
public:
    /*!
        \brief Check if the camera is specular.
    */
    virtual bool isSpecular(const PointGeometry& geom) const {
        LM_UNUSED(geom);
        LM_UNREACHABLE_RETURN();
    }

    /*!
        \brief Generate a primary ray with the corresponding raster position.
    */
    virtual Ray primaryRay(Vec2 rp) const {
        LM_UNUSED(rp);
        LM_UNREACHABLE_RETURN();
    }

    /*!
        \brief Sample a primary ray within the given raster window.
    */
    virtual std::optional<CameraRaySample> samplePrimaryRay(Rng& rng, Vec4 window) const {
        LM_UNUSED(rng, window);
        LM_UNREACHABLE_RETURN();
    }

    /*!
        \brief Evaluate importance.
    */
    virtual Vec3 eval(const PointGeometry& geom, Vec3 wo) const {
        LM_UNUSED(geom, wo);
        LM_UNREACHABLE_RETURN();
    }

    /*!
        \brief Get view matrix if available.
    */
    virtual Mat4 viewMatrix() const {
        LM_UNREACHABLE_RETURN();
    }

    /*!
        \brief Get projection matrix if available.
    */
    virtual Mat4 projectionMatrix() const {
        LM_UNREACHABLE_RETURN();
    }
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
