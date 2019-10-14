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
        \param geom Point geometry.

        \rst
        This function checks if the phase function contains delta component.
        \endrst
    */
    virtual bool isSpecular(const PointGeometry& geom) const = 0;

    /*!
        \brief Sample a direction.
        \param rng Random number generator.
        \param geom Point geometry.
        \param wi Incident ray direction.
        \return Sampled direction and associated information.
    */
    virtual std::optional<PhaseDirectionSample> sample(Rng& rng, const PointGeometry& geom, Vec3 wi) const = 0;

    /*!
        \brief Evaluate pdf in solid angle measure.
        \param geom Point geometry.
        \param wi Incident ray direction.
        \param wo Outgoing ray direction.
        \return Evaluated pdf.
    */
    virtual Float pdf(const PointGeometry& geom, Vec3 wi, Vec3 wo) const = 0;

    /*!
        \brief Evaluate the phase function.
        \param geom Point geometry.
        \param wi Incident ray direction.
        \param wo Outgoing ray direction.
        \return Evaluated value.
    */
    virtual Vec3 eval(const PointGeometry& geom, Vec3 wi, Vec3 wo) const = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
