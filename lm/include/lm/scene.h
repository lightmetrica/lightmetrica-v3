/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "detail/component.h"
#include "detail/forward.h"
#include "math.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \brief Primitive.
    A single object in the scene.
*/
struct ScenePrimitive : public Component {
    mat4 transform;
    const Component* mesh = nullptr;
    const Component* material = nullptr;
    const Component* sensor = nullptr;
    const Component* light = nullptr;
};

/*!
    \brief Scene.
*/
class Scene : public Component {
public:
    /*!
        \brief Loads a scene primitive.
    */
    virtual bool loadPrimitive(const std::string& name, const Assets& assets,
        mat4 transform, const json& prop) = 0;

    /*!
        \brief Build acceleration structure.
    */
    virtual void build(const std::string& name) = 0;
};

LM_NAMESPACE_END(LM_NAMESPACE)
