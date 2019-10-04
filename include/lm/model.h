/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "scenenode.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup model
    @{
*/

/*!
    \brief 3D model format.

    \rst
    The componet interface represents a 3D model
    that aggregates multiple meshes and materials.
    As well as meshes and materials, a model contains
    a set associations between meshes and materials,
    used to generate a set of scene primitives.
    See :ref:`making_scene` for detail.
    \endrst
*/
class Model : public Component {
public:
    /*!
        \brief Callback function to process a primitive.
        \param mesh Underlying mesh.
        \param material Underlying material.
        \param light Underlying light.

        \rst
        The function of this type is used as an argument of
        :cpp:func:`lm::Model::createPrimitives` function.
        \endrst
    */
    using CreatePrimitiveFunc = std::function<void(Component* mesh, Component* material, Component* light)>;

    /*!
        \brief Create primitives from underlying components.
        \param createPrimitive Callback function to be called for each primitive.

        \rst
        This function enumerates a set of primitives generated from the model.
        A specified callback function is called for each primitive.
        The function is internally used by the framework,
        so the users do not want to used it directly.
        \endrst
    */
    virtual void createPrimitives(const CreatePrimitiveFunc& createPrimitive) const = 0;

	/*!
	*/
	using VisitNodeFuncType = std::function<void(const SceneNode&)>;
	virtual void foreachNode(const VisitNodeFuncType& visit) const = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
