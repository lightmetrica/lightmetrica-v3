/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "detail/component.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \brief Collection of assets.
*/
class Assets : public Component {
public:
    /*!
        \brief Loads an asset.
    */
    virtual bool loadAsset(const std::string& name, const std::string& implKey, const Json& prop) = 0;

};

LM_NAMESPACE_END(LM_NAMESPACE)
