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

    For detail, please refer to :ref:`path_sampling`.
    \endrst
*/
class Material : public Component {
public:
    //! Light transport direction.
    enum class TransDir {
        LE,     //!< Light to camera.
        EL      //!< Camera to light.
    };

    // --------------------------------------------------------------------------------------------

    //! Result of component sampling.
    struct ComponentSample {
        int comp;
        Float weight;
    };

    //! Random number input for component sampling.
    struct ComponentSampleU {
        Vec2 uc;
    };

    /*!
        \brief Component sampling.
        \param u Random number input.
        \param geom Point geometry.

        \rst
        This function samples a component of the material
        :math:`j \sim p_{c,\mathrm{bsdf}}(\cdot\mid\mathbf{x})`.
        \endrst
    */
    virtual ComponentSample sample_component(const ComponentSampleU& u, const PointGeometry& geom) const = 0;
    
    /*!
        \brief Evaluate PDF for component sampling.
        \param comp Sampled component index.
        \param geom Point geometry.
       
        \rst
        This function evaluats :math:`p_{c,\mathrm{bsdf}}(j\mid\mathbf{x})`.
        \endrst
    */
    virtual Float pdf_component(int comp, const PointGeometry& geom) const = 0;

    // --------------------------------------------------------------------------------------------

    //! Result of direction sampling.
    struct DirectionSample {
        Vec3 wo;        //!< Sampled direction.
        Vec3 weight;    //!< Contribution divided by PDF.
    };

    //! Random number input for direction sampling.
    struct DirectionSampleU {
        Vec2 ud;
        Vec2 udc;
    };

    /*!
        \brief Direction sampling.
        \param u Random number input.
        \param geom Point geometry.
        \param wi Incident ray direction.
        \param comp Component index.
        \param trans_dir Transport direction.
		\return Sampled direction with associated information. nullopt for invalid sample.
        
        \rst
        This function samples a direction given a surface point, an incident direction, and a component index:
        :math:`\omega \sim p_{\sigma^* \mathrm{bsdf}}(\cdot\mid\mathbf{x},j)`.
        \endrst
    */
    virtual std::optional<DirectionSample> sample_direction(const DirectionSampleU& u, const PointGeometry& geom, Vec3 wi, int comp, TransDir trans_dir) const = 0;

    /*!
        \brief Evaluate pdf in projected solid angle measure.
        \param geom Point geometry.
        \param comp Component index.
        \param wi Incident ray direction.
        \param wo Sampled outgoing ray direction.
        \param comp Component index.
        \param eval_delta If true, evaluate delta function.

        \rst
        This function evaluates the PDF :math:`p_{\sigma^* \mathrm{bsdf}}(\omega\mid\mathbf{x},j)`.
        ``eval_delta`` flag enforces to evaluate delta function when the PDF contains one.
        \endrst
    */
    virtual Float pdf_direction(const PointGeometry& geom, Vec3 wi, Vec3 wo, int comp, bool eval_delta) const = 0;

    // --------------------------------------------------------------------------------------------

    /*!
        \brief Evaluate BSDF.
        \param geom Point geometry.
        \param comp Component index.
        \param wi Incident ray direction.
        \param wo Outgoing ray direction.

        \rst
        This function evaluates underlying BSDF :math:`f_{\mathrm{bsdf}\Omega}(\mathbf{x},j,\omega_i,\omega_o)`
        according to the transport direction ``trans_dir``.
        Note that ``trans_dir`` is necessary to support
        non-symmetric scattering described in Veach's thesis [Veach 1998, Chapter 5].
        \endrst
    */
    virtual Vec3 eval(const PointGeometry& geom, Vec3 wi, Vec3 wo, int comp, TransDir trans_dir, bool eval_delta) const = 0;

    /*!
        \brief Check if BSDF contains delta component.
        \param comp Component index.

        \rst
        This function evaluats true if the component of the BSDF specified by ``comp``
        contains a delta function.
        \endrst
    */
    virtual bool is_specular_component(int comp) const = 0;

    /*!
        \brief Evaluate reflectance.
        \param geom Point geometry.
        \param comp Component index.

        \rst
        This function evaluates the reflectance function of the underlying material if exists.
        \endrst
    */
    virtual Vec3 reflectance(const PointGeometry& geom) const = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
