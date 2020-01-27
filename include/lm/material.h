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
    \brief Material.

    \rst
    This component insterface represents a material of the scene object,
    responsible for sampling of reflected/refracted rays from the material,
    or the evaluation of BSDF.

    A material can contain multiple components.
    A component can be specified by an implementation-dependent component index.
    \endrst
*/
class Material : public Component {
public:
    /*!
        \brief Light transport direction.
    */
    enum class TransDir {
        LE,
        EL
    };

    /*!
        \brief Result of direction sampling.

        \rst
        This structure represents the result of
        :cpp:func:`lm::Material::sample` function.
        \endrst
    */
    struct DirectionSample {
        Vec3 wo;        //!< Sampled direction.
        Vec3 weight;    //!< Contribution divided by probability (including probability of component selection).
        bool specular;  //!< Sampled component is specular.
    };

    /*!
    */
    struct DirectionSampleU {
        Vec2 ud;
        Vec2 udc;
    };

    /*!
        \brief Sample a ray given surface point and incident direction.
        \param rng Random number generator.
        \param geom Point geometry.
        \param wi Incident ray direction.
		\return Sampled direction with associated information. nullopt for invalid sample.
        
        \rst
        This function samples direction based on the probability distribution associated to the material.
        Mathematically, this function samples a direction :math:`\omega` according to the pdf
        :math:`p(\omega\mid \mathbf{x}, \omega_i)` in projected solid angle measure,
        where :math:`\mathbf{x}` is a surface point and :math:`\omega_i` is an incident ray direction.

        If the material contains multiple component, this function also samples the component type.
        The selected component type is stored into :cpp:member:`lm::DirectionSample::comp`.
        Note that the evaluated weight doesn't contain the evaluation of the pdf of component selection.
        \endrst
    */
    virtual std::optional<DirectionSample> sample_direction(const DirectionSampleU& u, const PointGeometry& geom, Vec3 wi, TransDir trans_dir) const = 0;

    /*!
        \brief Evaluate pdf in projected solid angle measure.
        \param geom Point geometry.
        \param comp Component index.
        \param wi Incident ray direction.
        \param wo Outgoing ray direction.

        \rst
        This function evaluates a pdf corresponding to :cpp:func:`lm::Material::sample` function.
        Note that the evaluated pdf doesn't contain the probabilty of component selection.
        \endrst
    */
    virtual Float pdf_direction(const PointGeometry& geom, Vec3 wi, Vec3 wo, bool eval_delta) const = 0;

    /*!
        \brief Evaluate BSDF.
        \param geom Point geometry.
        \param comp Component index.
        \param wi Incident ray direction.
        \param wo Outgoing ray direction.

        \rst
        This function evaluates underlying BSDF of the material.
        \endrst
    */
    virtual Vec3 eval(const PointGeometry& geom, Vec3 wi, Vec3 wo, TransDir trans_dir, bool eval_delta) const = 0;

    /*!
    */
    virtual bool is_specular_any() const = 0;

    /*!
        \brief Evaluate reflectance.
        \param geom Point geometry.
        \param comp Component index.

        \rst
        This function evaluates the reflectance function of the underlying material if exists.
        \endrst
    */
    virtual std::optional<Vec3> reflectance(const PointGeometry& geom) const = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
