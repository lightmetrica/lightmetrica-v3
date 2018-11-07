/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include "component.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \brief Texture component interface.
*/
class Texture : public Component {
public:
    /*!
        \brief Evaluate color component of the texture.
    */
    virtual Vec3 eval(Vec2 t) const = 0;

    /*!
        \brief Evaluate alpha component of the texture.
    */
    virtual Float evalAlpha(Vec2 t) const = 0;

    /*!
        \brief Check if texture has alpha component.
    */
    virtual bool hasAlpha() const = 0;
};

LM_NAMESPACE_END(LM_NAMESPACE)
