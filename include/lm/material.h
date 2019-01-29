/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "math.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup material
    @{
*/

/*!
    \brief Result of Material::sample() function.
*/
struct MaterialDirectionSample {
    Vec3 wo;        // Sampled direction
    int comp;       // Sampled component
    Vec3 weight;    // Contribution divided by probability
};

/*!
    \brief Material.
*/
class Material : public Component {
public:
    /*!
        \brief Check if the material is specular.
    */
    virtual bool isSpecular(const PointGeometry& geom) const {
        LM_UNUSED(geom);
        LM_UNREACHABLE_RETURN();
    }

    /*!
        \brief Sample a ray given surface point and incident direction.
    */
    virtual std::optional<MaterialDirectionSample> sample(Rng& rng, const PointGeometry& geom, Vec3 wi) const {
        LM_UNUSED(rng, geom, wi);
        LM_UNREACHABLE_RETURN();
    }

    /*!
        \brief Evaluate reflectance.
    */
    virtual std::optional<Vec3> reflectance(const PointGeometry& geom) const {
        LM_UNUSED(geom);
        LM_UNREACHABLE_RETURN();
    }

    /*!
        \brief Evaluate pdf in projected solid angle measure.
    */
    virtual Float pdf(const PointGeometry& geom, Vec3 wi, Vec3 wo) const {
        LM_UNUSED(geom, wi, wo);
        LM_UNREACHABLE_RETURN();
    }

    /*!
        \brief Evaluate BSDF.
    */
    virtual Vec3 eval(const PointGeometry& geom, Vec3 wi, Vec3 wo) const {
        LM_UNUSED(geom, wi, wo);
        LM_UNREACHABLE_RETURN();
    }
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
