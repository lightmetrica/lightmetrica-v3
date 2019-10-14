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
    \addtogroup light
    @{
*/

/*!
    \brief Result of light sampling.
    
    \rst
    This structure represents the result of
    :cpp:func:`lm::Light::sample` function.
    \endrst
*/
struct LightRaySample {
    PointGeometry geom;   //!< Sampled geometry information.
    Vec3 wo;              //!< Sampled direction.
    int comp;             //!< Sampled component.
    Vec3 weight;          //!< Contribution divided by probability.
};

/*!
    \brief Light.

    \rst
    This component interface represents a light source inside the scene.
    responsible for sampling and evaluation of the rays emitted from/to the light source. 
    \endrst
*/
class Light : public Component {
public:
    /*!
        \brief Check if the light contains delta component.
        \param geom Surface geometry information.
        \param comp Component index.

        \rst
        This function returns true if the luminance function :math:`L_e` of the light source
        contains a component of delta function.
        \endrst
    */
    virtual bool isSpecular(const PointGeometry& geom, int comp) const = 0;

    /*!
        \brief Sample a position on the light.
        \param rng Random number generator.
        \param geom Point geometry on the scene surface.
        \param transform Transformation of the light source.

        \rst
        This function samples a direction from the surface point ``geom``
        toward the point on the light source.
        The transformation of the light source is specified by ``transform``.
        
        Mathmatically, the function samples a direction :math:`\omega`
        from the conditional probability density :math:`p(\omega\mid \mathbf{x}')`
        in projected solid angle measure given the surface point :math:`\mathbf{x}'`.

        For convenience, the function also returns the point on the light source
        in the direction of the ray :math:`(\mathbf{x}',\omega)`.
        If there is no corresponding surface geometry in the case of distant light source,
        ``geom.infinite`` becomes true.
        \endrst
    */
    virtual std::optional<LightRaySample> sample(Rng& rng, const PointGeometry& geom, const Transform& transform) const = 0;

    /*!
        \brief Evaluate pdf for light sampling in projected solid angle measure.
        \param geom Point geometry on the scene surface.
        \param geomL Point geometry on the light source.
        \param comp Component index.
        \param transform Transformation of the light source.
        \param wo Outgoing direction from the point of the light source.

        \rst
        This function evalutes a pdf corresponding to :cpp:func:`lm::Light::sample` function.
        ``geomL`` is either the point on the light source or inifite point.
        If the given direction cannot be sampled, the function returns zero.
        \endrst
    */
    virtual Float pdf(const PointGeometry& geom, const PointGeometry& geomL, int comp, const Transform& transform, Vec3 wo) const = 0;

    /*!
        \brief Check if the light source is inifite distant light.

        \rst
        This function checks if the light source is inifite distant light
        such as environment light or directional light.
        \endrst
    */
    virtual bool isInfinite() const = 0;

    /*!
        \brief Evaluate luminance.
        \param geom Point geometry on the light source.
        \param comp Component index.
        \param wo Outgoing direction from the point of the light source.

        \rst
        This function evaluates luminance function :math:`L_e` of the light source.
        \endrst
    */
    virtual Vec3 eval(const PointGeometry& geom, int comp, Vec3 wo) const = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
