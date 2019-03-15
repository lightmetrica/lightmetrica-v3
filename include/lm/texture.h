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

    \rst
    This structure represents a texture size
    used as a return type of :cpp:func:`lm::Texture::size` function.
    \endrst
*/
struct TextureSize {
    int w;      //<! Width of the texture.
    int h;      //<! Height of the texture.
};

/*!
    \brief Texture buffer.

    \rst
    The structure represents a film buffer
    used as a return type of :cpp:func:`lm::Texture::buffer` function.
    The data format of the buffer is implementation-dependent.
    We restrict the underlying data type of the buffer to float vector.
    \endrst
*/
struct TextureBuffer {
    int w;          //!< Width.
    int h;          //!< Height.
    int c;          //!< Components.
    float* data;    //!< Underlying data type.
};

/*!
    \brief Texture.

    \rst
    A component interface representing a texture
    used as an input of the materials.
    \endrst
*/
class Texture : public Component {
public:
    /*!
        \brief Get size of the texture.
        \return Size of the texture.
    */
    virtual TextureSize size() const = 0;

    /*!
        \brief Evaluate color component of the texture.
        \param t Texture coordinates.

        \rst
        This function evaluate color of the texture
        of the specified texture coordinates.
        Handling of the texture coodinates outside of
        the range :math:`[0,1]^2` is implementation-dependent.
        \endrst
    */
    virtual Vec3 eval(Vec2 t) const = 0;

    /*!
        \brief Evaluate color component of the texture by pixel coordinates.
        \param x x coordinate of the texture.
        \param y y coordinate of the texture.

        \rst
        This function evaluates color of the texture
        of the specified pixel coordinates.
        \endrst
    */
    virtual Vec3 evalByPixelCoords(int x, int y) const = 0;

    /*!
        \brief Evaluate alpha component of the texture.
        \param t Texture coordinates.
        
        \rst
        This evalutes alpha component of the texture.
        If the texture has no alpha component, the behavior is undefined.
        Use :cpp:func:`lm::Texture::hasAlpha` function to check
        if the texture has an alpha component.
        \endrst
    */
    virtual Float evalAlpha(Vec2 t) const { LM_UNUSED(t); return 0_f; }

    /*!
        \brief Check if texture has alpha component.
        
        \rst
        This function returns true if the texture has an alpha component.
        \endrst
    */
    virtual bool hasAlpha() const { return false; }

    /*!
        \brief Get buffer of the texture.
        \return Texture buffer.
    */
    virtual TextureBuffer buffer() { return {}; }
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
