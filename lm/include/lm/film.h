/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "detail/component.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \brief Film.
*/
class Film : public Component {
public:
    /*!
        \brief Save rendered film.
    */
    virtual bool save(const std::string& outpath) const = 0;
};

LM_NAMESPACE_END(LM_NAMESPACE)
