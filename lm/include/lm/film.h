/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "math.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

struct FilmSize {
    int w;
    int h;
};

/*!
    \brief Film.
*/
class Film : public Component {
public:
    /*!
        \brief Get size of the film.
    */
    virtual FilmSize size() const = 0;

    /*!
        \brief Set pixel value.
    */
    virtual void setPixel(int x, int y, Vec3 v) = 0;

    /*!
        \brief Save rendered film.
    */
    virtual bool save(const std::string& outpath) const = 0;

    /*!
        \brief Get aspect ratio.
    */
    Float aspectRatio() const {
        const auto [w, h] = size();
        return Float(w) / h;
    }
};

LM_NAMESPACE_END(LM_NAMESPACE)
