/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "math.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup phase
    @{
*/

/*!
    \brief Result of direction sampling.
*/
struct PhaseDirectionSample {
    Vec3 wo;        //!< Sampled direction.
    Vec3 weight;    //!< Contribution divided by probability.
};

/*!
    \brief Phase function.
*/
class Phase : public Component {
public:
    /*!
        \brief Check if the phase function is specular.
    */
    virtual bool isSpecular(const PointGeometry& geom) const = 0;

    /*!
        \brief Sample a direction.
    */
    virtual std::optional<PhaseDirectionSample> sample(Rng& rng, const PointGeometry& geom, Vec3 wi) const = 0;

    /*!
        \brief Evaluate pdf in solid angle measure.
    */
    virtual Float pdf(const PointGeometry& geom, Vec3 wi, Vec3 wo) const = 0;

    /*!
        \brief Evaluate the phase function.
    */
    virtual Vec3 eval(const PointGeometry& geom, Vec3 wi, Vec3 wo) const = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
