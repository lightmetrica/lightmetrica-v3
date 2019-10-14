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

// ------------------------------------------------------------------------------------------------

/*!
    \brief Scene.

    \rst
    This class represent a component interface for a scene.
    A scene is responsible for sampling of a ray emitted from a point inside a scene,
    evaluation of directional terms given a point in the scene,
    ray-scene intersection, visibility query, etc.
    The class is a basic building block to construct your own renderer.
    
    A scene is also responsible for managing the collection of assets (e.g., meshes, materials, etc.).
    Underlying assets are accessed via standard query functions like
    :cpp:func:`lm::Component::underlying`. The class provides the feature for internal usage
    and the user may not want to use this interface directly.
    The feature of the class is exposed by ``user`` namespace.
    \endrst
*/
class Scene : public Component {
public:
    /*!
        \brief Check if the scene is renderable.
        \return 

        \rst
        This function returns true if the scene is renderable.
        If not, the function returns false with error messages.
        \endrst
    */
    virtual bool renderable() const = 0;

    // --------------------------------------------------------------------------------------------

    /*!
        \brief Loads an asset.
        \param name Name of the asset.
        \param implKey Key of component implementation in `interface::implementation` format.
        \param prop Properties.
        \return Locator of the asset. nullopt if failed.

        \rst
        Loads an asset from the given information and registers to the class.
        ``implKey`` is used to create an instance and ``prop`` is used to construct it.
        ``prop`` is passed to :cpp:func:`lm::Component::construct` function of
        the implementation of the asset.

        If the asset with same name is already loaded, the function tries
        to deregister the previously-loaded asset and reload an asset again.
        If the global component hierarchy contains a reference to the original asset,
        the function automatically resolves the reference to the new asset.
        For usage, see ``functest/func_update_asset.py``.
        \endrst
    */
    virtual std::optional<std::string> loadAsset(const std::string& name, const std::string& implKey, const Json& prop) = 0;

    // --------------------------------------------------------------------------------------------

    /*!
        \brief Get index of the root node.
        \return Node index.
    */
    virtual int rootNode() = 0;

    /*!
        \brief Load scene node(s).
        \param type Scene node type.
        \param prop Property containing references to the scene components.
        \return Index of the created node.

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
        \param parent Parent node index.
        \param child Child node index being added.

        \rst
        This function adds a primitive group to the scene
        and returns index of the group.
        If the group with the same name is already created,
        this function returns the index of the registered group.
        \endrst
    */
    virtual void addChild(int parent, int child) = 0;

    /*!
        \brief Add child node from model asset.
        \param parent Index of the parent node.
        \param modelLoc Locator of the model asset.
    */
    virtual void addChildFromModel(int parent, const std::string& modelLoc) = 0;

    /*!
        \brief Create group node from model asset.
        \param modelLoc Locator of the model asset.
        \return Index of the created node.
    */
    virtual int createGroupFromModel(const std::string& modelLoc) = 0;

    // --------------------------------------------------------------------------------------------

    /*!
        \brief Callback function to traverse the scene nodes.
        \param node Current node.
        \param globalTransform Global transform applied to the node.
    */
    using NodeTraverseFunc = std::function<void(const SceneNode& node, Mat4 globalTransform)>;

    /*!
        \brief Iterate primitive nodes in the scene.
        \param traverseFunc Function being called for each traversed primitive node.
        
        \rst
        This function traverses the primitive nodes in the scene graph.
        For each primitive node, global transformation is computed and passed as
        an argument of the callback function.
        Note that this function does not traverse intermediate group nodes.
        If you want to traverse also the group node, consider to use :cpp:func:`Scene::visitNode`.
        \endrst
    */
    virtual void traversePrimitiveNodes(const NodeTraverseFunc& traverseFunc) const = 0;

    /*!
        \brief Callback function to traverse the scene nodes.
        \param node Scene node.
    */
    using VisitNodeFunc = std::function<void(const SceneNode& node)>;

    /*!
        \brief Traverse a node in the scene.
        \param nodeIndex Note index where the traverse starts.
        \param visit Callback function being called for each traversed node.

        \rst
        This function traverse a node in the scene graph.
        Unlike :cpp:func:`Scene::traversePrimitiveNodes`, this function
        can be used to traverse all kinds of scene nodes in the scene graph.
        Be careful the user is responsible to call this function to traverse the node recursively.
        \endrst
    */
    virtual void visitNode(int nodeIndex, const VisitNodeFunc& visit) const = 0;

    /*!
        \brief Get scene node by index.
        \param nodeIndex Scene node index.
        \return Scene node.
    */
    virtual const SceneNode& nodeAt(int nodeIndex) const = 0;

    // --------------------------------------------------------------------------------------------

    /*!
        \brief Build acceleration structure.
        \param name Name of the acceleration structure.
        \param prop Property for configuration.
    */
    virtual void build(const std::string& name, const Json& prop) = 0;

    /*!
        \brief Compute closest intersection point.
        \param ray Ray.
        \param tmin Lower bound of the valid range of the ray.
        \param tmax Upper bound of the valid range of the ray.
        
        \rst
        This function computes closest intersection point between the given ray and the scene
        utilizing underlying acceleration structure of the scene.
        If no intersection happens, this function returns ``nullopt``.
        Note that if the scene contains environment light, this function returns scene intersection structure
        indicating the intersection with infinite point.
        This can be examined by checking :cpp:member:`PointGeometry::infinite` being ``true``.
        \endrst
    */
    virtual std::optional<SceneInteraction> intersect(Ray ray, Float tmin = Eps, Float tmax = Inf) const = 0;

    /*!
        \brief Check if two surface points are mutually visible.
        \param sp1 Scene interaction of the first point.
        \param sp2 Scene interaction of the second point.
        \return Returns true if two points are mutually visible, otherwise returns false.
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

    // --------------------------------------------------------------------------------------------

    /*!
        \brief Check if given surface point is light.
        \param sp Scene interaction.
        \return True if scene interaction is light.
    */
    virtual bool isLight(const SceneInteraction& sp) const = 0;

    /*!
        \brief Check if given surface point is specular.
        \param sp Scene intersection.
        \return True if scene interaction is specular.

        \rst
        Scene interaction is specular if the material, light, or camera associated
        with point specified by scene intersection contains delta function.
        \endrst
    */
    virtual bool isSpecular(const SceneInteraction& sp) const = 0;

    // --------------------------------------------------------------------------------------------

    /*!
        \brief Generate a primary ray.
        \param rp Raster position in [0,1]^2.
        \param aspectRatio Aspect ratio of the film.
        \return Generated primary ray.

        \rst
        This function deterministically generates a primary ray
        corresponding to the given raster position.
        \endrst
    */
    virtual Ray primaryRay(Vec2 rp, Float aspectRatio) const = 0;

    /*!
        \brief Compute a raster position.
        \param wo Primary ray direction.
        \param aspectRatio Aspect ratio of the film.
        \return Raster position.
    */
    virtual std::optional<Vec2> rasterPosition(Vec3 wo, Float aspectRatio) const = 0;
    
    /*!
        \brief Sample a ray given surface point and incident direction.
        \param rng Random number generator.
        \param sp Surface interaction.
        \param wi 

        \rst
        This function samples a ray given the scene interaction.
        According to the types of scene interaction, this function samples a different
        types of the ray from the several distributions.

        (1) If the scene interaction is ``terminator``, this function samples a primary ray
            according type types of the terminator (camera or light). ``wi`` is ignored in this case.

        (2) If the scene interaction is not ``terminator``, this function samples
            a ray from the associated distribution to BSDF or phase function
            given the interaction event ``sp`` and incident ray direction ``wi``.

        In both cases, this function returns ``nullopt`` if the sampling failed,
        or the case when the early return is possible for instance when 
        the evaluated contribution of the sampled direction is zero.
        \endrst
    */
    virtual std::optional<RaySample> sampleRay(Rng& rng, const SceneInteraction& sp, Vec3 wi) const = 0;

    /*!
        \brief Sample direction to a light given a scene interaction.
        \param rng Random number generator.
        \param sp Scene interaction.

        \rst
        This function samples a ray to the light given a scene interaction.
        Be careful not to confuse the sampled ray with the ray sampled via :cpp:func:`Scene::sampleRay`
        function from a light source. Both rays are sampled from the different distributions
        and if you want to evaluate densities you want to use different functions.
        \endrst
    */
    virtual std::optional<RaySample> sampleDirectLight(Rng& rng, const SceneInteraction& sp) const = 0;

    /*!
        \brief Evaluate pdf for direction sampling.
        \param sp Scene interaction.
        \param wi Incident ray direction.
        \param wo Sampled outgoing ray direction.
        \return Evaluated pdf.

        \rst
        This function evaluates pdf according to *projected solid angme measure* if ``sp.geom.degenerated=false``
        and *solid angme measure* if ``sp.geom.degenerated=true``
        utlizing corresponding densities from which the direction is sampled.
        \endrst
    */
    virtual Float pdf(const SceneInteraction& sp, Vec3 wi, Vec3 wo) const = 0;

    /*!
        \brief Evaluate pdf for component selection.
        \param sp Scene interaction.
        \param wi Incident ray direction.
    */
    virtual Float pdfComp(const SceneInteraction& sp, Vec3 wi) const = 0;

    /*!
        \brief Evaluate pdf for light sampling given a scene interaction.
        \param sp Scene interaction.
        \param spL Sampled scene interaction of the light.
        \param wo Sampled outgoing ray directiom *from* the light.

        \rst
        This function evaluate pdf for the ray sampled via :cpp:func:`Scene::sampleDirectLight`.
        Be careful ``wo`` is the outgoing direction originated from ``spL``, not ``sp``.
        \endrst
    */
    virtual Float pdfDirectLight(const SceneInteraction& sp, const SceneInteraction& spL, Vec3 wo) const = 0;

    // --------------------------------------------------------------------------------------------

    /*!
        \brief Sample a distance in a ray direction.
        \param rng Random number generator.
        \param sp Scene interaction.
        \param wo Ray direction.

        \rst
        This function samples either a point in a medium or a point on the surface.
        Note that we don't provide corresponding pdf function because 
        some underlying distance sampling technique might not have the analitical representation.
        \endrst
    */
    virtual std::optional<DistanceSample> sampleDistance(Rng& rng, const SceneInteraction& sp, Vec3 wo) const = 0;

    /*!
        \brief Evaluate transmittance.
        \param rng Random number generator.
        \param sp1 Scene interaction of the first point.
        \param sp2 Scene interaction of the second point.
        
        \rst
        This function evaluates transmittance between two scene interaction events.
        This function might need a random number generator because heterogeneous media needs stochastic estimation.
        If the space between ``sp1`` and ``sp2`` is vacuum (i.e., no media),
        this function is conceptually equivalent to :cpp:func:`Scene::visible`.
        \endrst
    */
    virtual Vec3 evalTransmittance(Rng& rng, const SceneInteraction& sp1, const SceneInteraction& sp2) const = 0;

    // --------------------------------------------------------------------------------------------

    /*!
        \brief Evaluate contribution.
        \param sp Scene interaction.
        \param wi Incident ray direction.
        \param wo Outgoing ray direction.
        \return Evaluated contribution.

        \rst
        This function evaluate directional contribution according to the scene interaction type.
        
        (1) If the scene interaction is endpoint and on a light,
            this function evaluates luminance function.

        (2) If the scene interaction is endpoint and on a sensor,
            this function evaluates importance function.
        
        (3) If the scene interaction is not endpoint and on a surface,
            this function evaluates BSDF.

        (4) If the scene interaction is in a medium,
            this function evaluate phase function.

        Note that the scene interaction obtained from :cpp:func:`Scene::intersect` or 
        :cpp:func:`Scene::sampleDistance` is not an endpont
        even if it might represent either a light or a sensor.
        In this case, you want to use :cpp:func:`Scene::evalContrbEndpoint`
        to enforce an evaluation as an endpoint.
        \endrst
    */
    virtual Vec3 evalContrb(const SceneInteraction& sp, Vec3 wi, Vec3 wo) const = 0;

    /*!
        \brief Evaluate endpoint contribution.

        \rst
        This function evaluates

        (1) If the scene interaction *contains* a light component,
            this function evaluates luminance function.

        (2) If the scene interaction *contains* a sensor component,
            this function evaluates importance function.

        That is, this function enforces the evaluation as an endpoint
        irrespective to the value of ``sp.endpoint``.
        \endrst
    */
    virtual Vec3 evalContrbEndpoint(const SceneInteraction& sp, Vec3 wo) const = 0;

    /*!
        \brief Evaluate reflectance (if available).
        \param sp Surface interaction.

        \rst
        This function evaluate reflectance if ``sp`` is on a surface
        and the associated material implements :cpp:func:`Material::reflectance` function.
        \endrst
    */
    virtual std::optional<Vec3> reflectance(const SceneInteraction& sp) const = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
