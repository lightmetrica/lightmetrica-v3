/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "math.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup film
    @{
*/

/*!
    \brief Film size.

    \rst
    This structure represents a film size
    used as a return type of :cpp:func:`lm::Film::size` function.
    \endrst
*/
struct FilmSize {
    int w;      //!< Width of the film.
    int h;      //!< Height of the film.
};

/*!
    \brief Film buffer.

    \rst
    The structure represents a film buffer
    used as a return type of :cpp:func:`lm::Film::buffer` function.
    The data format of the buffer is implementation-dependent.
    \endrst
*/
struct FilmBuffer {
    int w;          //!< Width of the buffer.
    int h;          //!< Height of the buffer.
    Float* data;    //!< Data.
};

/*!
    \brief Film.

    \rst
    A component interface representing a film.
    The component is used as an output from renderers.
    \endrst
*/
class Film : public Component {
public:
    /*!
        \brief Get size of the film.
        \return Size of the film.
    */
    virtual FilmSize size() const = 0;

    /*!
        \brief Set pixel value.
        \param x x coordinate of the film.
        \param y y coordinate of the film.
        \param v Pixel color.
        
        \rst
        This function sets the pixel color to the specified pixel coordinates ``(x,y)``.
        This function is thread-safe and safe to be accessed from multiple threads
        in the process of rendering.
        \endrst
    */
    virtual void setPixel(int x, int y, Vec3 v) = 0;

    /*!
        \brief Save rendered film.
        \param outpath Output image path.

        \rst
        This function saves the internal represention of the film to the specified path.
        The output image format is implementation-dependent.
        If it fails to save the film, the function returns false.
        \endrst
    */
    virtual bool save(const std::string& outpath) const = 0;

    /*!
        \brief Get aspect ratio.
        \return Aspect ratio.
    */
    Float aspectRatio() const {
        const auto [w, h] = size();
        return Float(w) / h;
    }

    /*!
        \brief Get buffer of the film
        \return Film buffer.

        \rst
        The function returns film buffer.
        The buffer is allocated internally and becomes invalid if the film is deleted.
        The data format of the buffer is implementation-dependent.
        \endrst
    */
    virtual FilmBuffer buffer() = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
