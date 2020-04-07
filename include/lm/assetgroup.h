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
    \brief Asset group.

    \rst
    This class represents a collection of the assets (e.g., materials, meshes, etc.).
    The assets in the framework are stored in this class.
    \endrst
*/
class AssetGroup : public Component {
public:
    /*!
        \brief Loads an asset.
        \param name Name of the asset.
        \param impl_key Key of component implementation in `interface::implementation` format.
        \param prop Properties.
        \return Pointer to the created asset. nullptr if failed.

        \rst
        Loads an asset from the given information and registers to the class.
        ``impl_key`` is used to create an instance and ``prop`` is used to construct it.
        ``prop`` is passed to :cpp:func:`lm::Component::construct` function of
        the implementation of the asset.
        This function returns a pointer to the created instance.
        If failed, it returns nullptr.

        If the asset with same name is already loaded, the function tries
        to deregister the previously-loaded asset and reload an asset again.
        If the global component hierarchy contains a reference to the original asset,
        the function automatically resolves the reference to the new asset.
        For usage, see ``functest/func_update_asset.py``.
        \endrst
    */
    virtual Component* load_asset(const std::string& name, const std::string& impl_key, const Json& prop) = 0;

    /*!
        \brief Load a serialized asset.
        \param name Name of the asset.
        \param path Path to the serialized asset.
    */
    virtual Component* load_serialized(const std::string& name, const std::string& path) = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
