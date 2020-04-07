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
    \brief Phase function.
*/
class Phase : public Component {
public:
    //! Result of direction sampling.
    struct DirectionSample {
        Vec3 wo;            //!< Sampled direction.
        Vec3 weight;        //!< Contribution divided by probability.
    };

    //! Random number input for direction sampling.
    struct DirectionSampleU {
        Vec2 ud;
    };

    /*!
        \brief Sample a direction.
        \param u Random number input.
        \param geom Point geometry.
        \param wi Incident ray direction.
        \return Sampled direction and associated information.
    */
    virtual std::optional<DirectionSample> sample_direction(const DirectionSampleU& u, const PointGeometry& geom, Vec3 wi) const = 0;

    /*!
        \brief Evaluate pdf in solid angle measure.
        \param geom Point geometry.
        \param wi Incident ray direction.
        \param wo Outgoing ray direction.
        \return Evaluated pdf.
    */
    virtual Float pdf_direction(const PointGeometry& geom, Vec3 wi, Vec3 wo) const = 0;

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
