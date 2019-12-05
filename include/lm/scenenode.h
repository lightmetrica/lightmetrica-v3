/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "math.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

enum class SceneNodeType {
    Primitive,
    Group,
};

/*!
    \brief Scene primitive.

    \rst
    This structure represents a scene primitive. The scene is described by a set of primitives
    and a primitive describes an object in the scene,
    which associates various scene components like mesh or material.
    A primitive can represent three types of scene objects.

    (1) *Scene geometry*.
        If ``mesh != nullptr`` and ``material != nullptr``,
        the structure describes a geometry in the scene,
        represented by an association of a mesh and a material.
        A transformation is applied to the mesh.

    (2) *Light*.
        If ``light != nullptr``, the structure describes a light in the scene.
        Note that a light can also be a scene geometry, such as area lights.

    (3) *Camera*.
        If ``camera != nullptr``, the structure describes a camera in the scene.
        Note that a camera and a light cannot be the same primitive,
        that is, ``light`` and ``camera`` cannot be non-``nullptr`` in the same time.

    A set of primitives is managed internally by the implementation of :cpp:class:`lm::Scene`
    and the class exposes a facade for the sampling and evaluation functions
    for the underlying component interfaces.
    Thus the users usually do not need to explicitly access the underlying
    component interfaces of a primitive.
    \endrst
*/
struct SceneNode {
    SceneNodeType type;                     //!< Scene node type.
    int index;                              //!< Node index.

    // --------------------------------------------------------------------------------------------
    
    //! Variables available for Primitive type.
	struct {
		Mesh* mesh = nullptr;               //!< Underlying mesh.
		Material* material = nullptr;       //!< Underlying material.
		Light* light = nullptr;             //!< Underlying light.
		Camera* camera = nullptr;           //!< Underlying camera.
		Medium* medium = nullptr;           //!< Underlying medium.

		//! \cond
		template <typename Archive>
		void serialize(Archive& ar) {
			ar(mesh, material, light, camera, medium);
		}
		//! \endcond
	} primitive;

    // --------------------------------------------------------------------------------------------

    //! Variable available for Group type.
    struct {
        std::vector<int> children;			    //!< Child primitives.
        bool instanced;							//!< True if the group is an instance group.
        std::optional<Mat4> local_transform;	//!< Transformation applied to children.

        //! \cond
        template <typename Archive>
        void serialize(Archive& ar) {
            ar(children, instanced, local_transform);
        }
        //! \endcond
    } group;

    // --------------------------------------------------------------------------------------------

    //! \cond
    template <typename Archive>
    void serialize(Archive& ar) {
        ar(type, index, primitive, group);
    }
    //! \endcond

    // --------------------------------------------------------------------------------------------

    /*!
        \brief Make primitive node.
        \param index Primitive index.
        \param mesh Reference to mesh.
        \param material Reference to material.
        \param light Reference to light.
        \param camera Reference to camera.
        \param medium Reference to medium.
    */
    static SceneNode make_primitive(int index, Mesh* mesh, Material* material, Light* light, Camera* camera, Medium* medium) {
        SceneNode p;
        p.type = SceneNodeType::Primitive;
        p.index = index;
        p.primitive.mesh = mesh;
        p.primitive.material = material;
        p.primitive.light = light;
        p.primitive.camera = camera;
        p.primitive.medium = medium;
        return p;
    }

    /*!
        \brief Make group node.
        \param index Primitive index.
        \param instanced Node is instancing group if true.
        \param local_transform Local transform applied to the node.
    */
    static SceneNode make_group(int index, bool instanced, std::optional<Mat4> local_transform) {
        SceneNode p;
        p.type = SceneNodeType::Group;
        p.index = index;
        p.group.instanced = instanced;
        p.group.local_transform = local_transform;
        return p;
    }
};

LM_NAMESPACE_END(LM_NAMESPACE)
