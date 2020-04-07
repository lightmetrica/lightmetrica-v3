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
        \brief Get the number of pixels.
    */
    virtual long long num_pixels() const = 0;

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
    virtual void set_pixel(int x, int y, Vec3 v) = 0;

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
        \brief Get buffer of the film
        \return Film buffer.

        \rst
        The function returns film buffer.
        The buffer is allocated internally and becomes invalid if the film is deleted.
        The data format of the buffer is implementation-dependent.
        \endrst
    */
    virtual FilmBuffer buffer() = 0;

    /*!
        \brief Accumulate another film.
        \param film Another film.
    */
    virtual void accum(const Film* film) = 0;

    /*!
        \brief Splat the color to the film by pixel coordinates.
        \param x x coordinate of the film.
        \param y y coordinate of the film.
        \param v Color.
    */
    virtual void splat_pixel(int x, int y, Vec3 v) = 0;

    /*!
        \brief Callback function for updating a pixel value.
    */
    using PixelUpdateFunc = std::function<Vec3(Vec3 curr)>;

    /*!
        \brief Atomically update a pixel value based on the current value.

        \rst
        This function is useful to implement user-defined atomic operation
        to update a pixel value. The given function might be called more than once.
        \endrst
    */
    virtual void update_pixel(int x, int y, const PixelUpdateFunc& updateFunc) = 0;

    /*!
        \brief Rescale the film.
        \param s Scale.
    */
    virtual void rescale(Float s) = 0;

    /*!
        \brief Clear the film.
    */
    virtual void clear() = 0;

public:
    /*!
        \brief Get aspect ratio.
        \return Aspect ratio.
    */
    Float aspect() const {
        const auto [w, h] = size();
        return Float(w) / h;
    }

    /*!
        \brief Convert raster position to pixel coordinates.
        \param rp Raster position in [0,1]^2.
        \return Pixel coordinates.

        \rst
        This function converts the raster position to the pixel coordinates.
        If the raster position is outside of the range [0,1]^2,
        the pixel coodinates are clamped to [0,w-1]\times [0,h-1].
        \endrst
    */
    glm::ivec2 raster_to_pixel(Vec2 rp) const {
        const auto [w, h] = size();
        const int x = glm::clamp((int)(rp.x * Float(w)), 0, w-1);
        const int y = glm::clamp((int)(rp.y * Float(h)), 0, h-1);
        return {x, y};
    }

    /*!
        \brief Incrementally accumulate average of a pixel value.
        \param x x coordinate of the film.
        \param y y coordinate of the film.
        \param index Current sample index.
        \param v Color.
    */
    void inc_ave(int x, int y, long long index, Vec3 v) {
        update_pixel(x, y, [&](Vec3 curr) -> Vec3 {
            return curr + (v - curr) / (Float)(index + 1);
        });
    }

    /*!
        \brief Incrementally accumulate average of a pixel value.
        \param rp Raster position.
        \param index Current sample index.
        \param v Color.
    */
    void inc_ave(Vec2 rp, long long index, Vec3 v) {
        const auto p = raster_to_pixel(rp);
        inc_ave(p.x, p.y, index, v);
    }

    /*!
        \brief Splat the color to the film.
        \param rp Raster position.
        \param v Color.
    */
    virtual void splat(Vec2 rp, Vec3 v) {
        const auto p = raster_to_pixel(rp);
        splat_pixel(p.x, p.y, v);
    }
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
