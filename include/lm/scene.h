/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "math.h"
#include "surface.h"
#include "scenenode.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// ----------------------------------------------------------------------------

/*!
    \addtogroup scene
    @{
*/

/*!
    \brief Result of ray sampling.

    \rst
    This structure represents the result of ray sampling
    used by the functions of :cpp:class:`lm::Scene` class.
    \endrst
*/
struct RaySample {
    SceneInteraction sp;   //!< Surface point information.
    Vec3 wo;               //!< Sampled direction.
    Vec3 weight;           //!< Contribution divided by probability.

    /*!
        \brief Get a ray from the sample.
        
        \rst
        This function constructs a :cpp:class:`lm::Ray`
        structure from the ray sample.
        \endrst
    */
    Ray ray() const {
        assert(!sp.geom.infinite);
        return { sp.geom.p, wo };
    }
};

/*!
    \brief Result of distance sampling.
*/
struct DistanceSample {
    SceneInteraction sp;    //!< Sampled interaction point.
    Vec3 weight;            //!< Contribution divided by probability.
};

// ----------------------------------------------------------------------------

/*!
    \brief Scene.

    \rst
    This class represent a component interface for a scene.
    A scene is responsible for sampling of a ray emitted from a point inside a scene,
    evaluation of directional terms given a point in the scene,
    ray-scene intersection, visibility query, etc.
    The class is a basic building block to construct your own renderer.
    For detail, see TODO.
    \endrst
*/
class Scene : public Component {
public:
    /*!
        \brief Check if the scene is renderable.

        \rst
        This function returns true if the scene is renderable.
        If not, the function returns false with error messages.
        \endrst
    */
    virtual bool renderable() const = 0;

    // ------------------------------------------------------------------------

    /*!
    */
    virtual int rootNode() = 0;

    /*!
        \brief Load scene node(s).
        \param groupPrimitiveIndex Group index.
        \param transform Transformation associated to the primitive.
        \param prop Property containing references to the scene components.

        \rst
        This function construct a primitive and add it to the scene
        given the transformation and the references specified in ``prop``.
        The type of the primitive created by this function changes according to
        the properties in ``prop``.
        The function returns true if the loading is a success.
        \endrst
    */
    virtual int createNode(SceneNodeType type, const Json& prop) = 0;

    /*!
        \brief Add primitive group.
        \param groupName Name of the group.

        \rst
        This function adds a primitive group to the scene
        and returns index of the group.
        If the group with the same name is already created,
        this function returns the index of the registered group.
        \endrst
    */
    virtual void addChild(int parent, int child) = 0;

    /*!
    */
    virtual void addChildFromModel(int parent, const std::string& modelLoc) = 0;

	/*!
	*/
	virtual int createGroupFromModel(const std::string& modelLoc) = 0;

    // ------------------------------------------------------------------------

    /*!
        \brief Iterate primitives in the scene.
    */
    using NodeTraverseFunc = std::function<void(const SceneNode& node, Mat4 globalTransform)>;
    virtual void traverseNodes(const NodeTraverseFunc& traverseFunc) const = 0;

    /*!
    */
    using VisitNodeFunc = std::function<void(const SceneNode& node)>;
    virtual void visitNode(int nodeIndex, const VisitNodeFunc& visit) const = 0;

    /*!
    */
    virtual const SceneNode& nodeAt(int nodeIndex) const = 0;

    // ------------------------------------------------------------------------

    /*!
        \brief Build acceleration structure.
        \param name Name of the acceleration structure.
        \param prop Property for configuration.
    */
    virtual void build(const std::string& name, const Json& prop) = 0;

    /*!
        \brief Compute closest intersection point.
    */
    virtual std::optional<SceneInteraction> intersect(
        Ray ray, Float tmin = Eps, Float tmax = Inf) const = 0;

    /*!
        \brief Check if two surface points are mutually visible.
    */
    bool visible(const SceneInteraction& sp1, const SceneInteraction& sp2) const {
        const auto visible_ = [this](const SceneInteraction& sp1, const SceneInteraction& sp2) -> bool {
            assert(!sp1.geom.infinite);
            const auto wo = sp2.geom.infinite
                ? -sp2.geom.wo
                : glm::normalize(sp2.geom.p - sp1.geom.p);
            const auto tmax = sp2.geom.infinite
                ? Inf - 1_f
                : [&]() {
                    const auto d = glm::distance(sp1.geom.p, sp2.geom.p);
                    return d * (1_f - Eps);
                }();
            // Exclude environent light from intersection test with tmax < Inf
            return !intersect(Ray{sp1.geom.p, wo}, Eps, tmax);
        };
        if (sp1.geom.infinite) {
            return visible_(sp2, sp1);
        }
        else {
            return visible_(sp1, sp2);
        }
    }

    // ------------------------------------------------------------------------

    /*!
        \brief Check if given surface point is light.
    */
    virtual bool isLight(const SceneInteraction& sp) const = 0;

    /*!
        \brief Check if given surface point is specular.
    */
    virtual bool isSpecular(const SceneInteraction& sp) const = 0;

    // ------------------------------------------------------------------------

    /*!
        \brief Generate a primary ray.
    */
    virtual Ray primaryRay(Vec2 rp, Float aspectRatio) const = 0;

    /*!
        \brief Sample a ray given surface point and incident direction.
        \rst
        (x,wo) ~ p(x,wo|sp,wi)
        \endrst
    */
    virtual std::optional<RaySample> sampleRay(Rng& rng, const SceneInteraction& sp, Vec3 wi) const = 0;

    /*!
        \brief Sample a ray given pixel position.
        \rst
        (x,wo) ~ p(x,wo|raster window)
        \endrst
    */
    virtual std::optional<RaySample> samplePrimaryRay(Rng& rng, Vec4 window, Float aspectRatio) const = 0;

    /*!
        \brief Sample a position on a light.
    */
    virtual std::optional<RaySample> sampleLight(Rng& rng, const SceneInteraction& sp) const = 0;

    /*!
        \brief Evaluate pdf for direction sampling.
    */
    virtual Float pdf(const SceneInteraction& sp, Vec3 wi, Vec3 wo) const = 0;

    /*!
        \brief Evaluate pdf for light sampling.
    */
    virtual Float pdfLight(const SceneInteraction& sp, const SceneInteraction& spL, Vec3 wo) const = 0;

    // ------------------------------------------------------------------------

    /*!
        \brief Sample a distance in a ray direction.
    */
    virtual std::optional<DistanceSample> sampleDistance(Rng& rng, const SceneInteraction& sp, Vec3 wo) const = 0;

    /*!
        \brief Evaluate transmittance.
    */
    virtual std::optional<Vec3> evalTransmittance(Rng& rng, const SceneInteraction& sp1, const SceneInteraction& sp2) const = 0;

    // ------------------------------------------------------------------------

    /*!
        \brief Evaluate contribution.
        \rst
        This function evaluates either BSDF for surface interaction
        or phase function for medium interaction.
        \endrst
    */
    virtual Vec3 evalContrb(const SceneInteraction& sp, Vec3 wi, Vec3 wo) const = 0;

    /*!
        \brief Evaluate endpoint contribution.
        \rst
        f(x,wo) where x is endpoint
        \endrst
    */
    virtual Vec3 evalContrbEndpoint(const SceneInteraction& sp, Vec3 wo) const = 0;

    /*!
        \brief Evaluate reflectance (if available).
        \rst
        \rho(x)
        \endrst
    */
    virtual std::optional<Vec3> reflectance(const SceneInteraction& sp) const = 0;
};

/*!
    @}
*/

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(LM_NAMESPACE)
