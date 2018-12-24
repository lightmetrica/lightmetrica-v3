/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include "component.h"
#include "math.h"

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
	virtual Float evalAlpha(Vec2 t) const { LM_UNUSED(t); return 0_f; }

    /*!
        \brief Check if texture has alpha component.
    */
    virtual bool hasAlpha() const { return false; }
};

LM_NAMESPACE_END(LM_NAMESPACE)
