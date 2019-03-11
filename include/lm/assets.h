/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup assets
    @{
*/

/*!
    \brief Collection of assets.

    \rst
    Interfaces collection of assets. All instances of the assets
    (e.g., meshes, materials, etc.) of the framework are managed by this class.
    Underlying assets are accessed via standard query functions like
    :cpp:func:`lm::Component::underlying`. The class provides the feature for internal usage
    and the user may not want to use this interface directly.
    The feature of the class is exposed by ``user`` namespace.
    \endrst
*/
class Assets : public Component {
public:
    /*!
        \brief Loads an asset.
        \param name Name of the asset.
        \param implKey Key of component implementation in `interface::implementation` format.
        \param prop Properties.

        \rst
        Loads an asset from the given information and registers to the class.
        ``implKey`` is used to create an instance and ``prop`` is used to construct it.
        ``prop`` is passed to :cpp:func:`lm::Component::construct` function of
        the implementation of the asset.

        If the asset with same name is already loaded, the function tries
        to deregister the previously-loaded asset and reload an asset again.
        If the global component hierarchy contains a reference to the original asset,
        the function automatically resolves the reference to the new asset.
        For usage, see ``functest/func_update_asset.py``.
        \endrst
    */
    virtual bool loadAsset(const std::string& name, const std::string& implKey, const Json& prop) = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
