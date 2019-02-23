/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include "component.h"
#include "math.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup texture
    @{
*/

/*!
    \brief Texture size.
*/
struct TextureSize {
    int w;
    int h;
};

/*!
    \brief Texture buffer.
*/
struct TextureBuffer {
    int w;       // WIdth
    int h;       // Height
    int c;       // Components
    float* data; // Underlying data type is already float
};

/*!
    \brief Texture component interface.
*/
class Texture : public Component {
public:
    /*!
        \brief Get size of the texture.
    */
    virtual TextureSize size() const = 0;

    /*!
        \brief Evaluate color component of the texture.
    */
    virtual Vec3 eval(Vec2 t) const = 0;

    /*!
        \brief Evaluate color component of the texture by pixel coordinates.
    */
    virtual Vec3 evalByPixelCoords(int x, int y) const = 0;

    /*!
        \brief Evaluate alpha component of the texture.
    */
    virtual Float evalAlpha(Vec2 t) const { LM_UNUSED(t); return 0_f; }

    /*!
        \brief Check if texture has alpha component.
    */
    virtual bool hasAlpha() const { return false; }

    /*!
        \brief Get buffer of the texture.
    */
    virtual TextureBuffer buffer() { return {}; }
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
