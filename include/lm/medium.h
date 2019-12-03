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
    bool medium;    //!< True if the point is is medium.
};

/*!
    \brief Participating medium.
*/
class Medium : public Component {
public:
    /*!
        \brief Sample a distance in a ray direction.
        \param rng Random number generator.
        \param ray Ray.
        \param tmin Lower bound of the valid range of the ray.
        \param tmax Upper bound of the valid range of the ray.

        \rst
        This function samples a scattering event in the valid range of the ray segmnet.
        Note that this function assumes there is no scene surfaces
        in the given range of the ray segment.
        If it does not sample a scattering event, this function returns ``nullopt``.
        \endrst
    */
    virtual std::optional<MediumDistanceSample> sample_distance(Rng& rng, Ray ray, Float tmin, Float tmax) const = 0;

    /*!
        \brief Evaluate transmittance.

        \rst
        This function estimates transmittance of the given ray segment.
        Note that this function assumes there is no scene surfaces
        in the given range of the ray segment.
        This function might need a random number generator
        because heterogeneous media needs stochastic estimation.
        \endrst
    */
    virtual Vec3 eval_transmittance(Rng& rng, Ray ray, Float tmin, Float tmax) const = 0;

    /*!
        \brief Check if the medium has emissive component.
        \return True if the participating media contains emitter.
    */
    virtual bool is_emitter() const = 0;

    /*!
        \brief Get underlying phase function.
        \return PHase function.
    */
    virtual const Phase* phase() const = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
