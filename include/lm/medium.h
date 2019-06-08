/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "math.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup medium
    @{
*/

/*!
    \brief Result of distance sampling.
*/
struct MediumDistanceSample {
    Vec3 p;         //!< Sampled point.
    Vec3 weight;    //!< Contribution divided by probability.
};

/*!
    \brief Participating medium.
*/
class Medium : public Component {
public:
    /*!
        \brief Sample a distance in a ray direction.
    */
    virtual std::optional<MediumDistanceSample> sampleDistance(Rng& rng, const PointGeometry& geom, Vec3 wo, Float distToSurf) const = 0;

    /*!
        \brief Check if the medium has emissive component.
    */
    virtual bool isEmitter() const = 0;

    /*!
        \brief Get underlying phase function.
    */
    virtual const Phase* phase() const = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
