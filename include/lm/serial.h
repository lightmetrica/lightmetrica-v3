/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include "component.h"
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/unordered_map.hpp>

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(cereal)

/*!
    \brief Save function specialized for component pointer.
    \tparam Archive Output archive type.
    \tparam T Component interface type.
*/
template <class Archive, class T>
void save(Archive& ar, const lm::Component::Ptr<T>& p) {
    // Save an additional information if the pointer is nullptr
    if (!p) {
        // The pointer is invalid
        ar(CEREAL_NVP_("valid", uint8_t(0)));
    }
    else {
        // The pointer is valid
        ar(CEREAL_NVP_("valid", uint8_t(1)));

        // Meta information needed to recreate the instance
        ar(CEREAL_NVP_("key", lm::comp::detail::Access::key(p.get())));

        // Save the contants with Component::save() function.
        // We don't use cereal's polymorphinc class support
        // because their features can be achievable with our component system.
        p->save(ar);
    }
}

/*!
    \brief Load function specialized for component pointer.
    \tparam Archive Output archive type.
    \tparam T Component interface type.
*/
template <class Archive, class T>
void load(Archive& ar, lm::Component::Ptr<T>& p) {
    uint8_t valid;
    ar(CEREAL_NVP_("valid", valid));
    if (!valid) {
        p.reset();
    }
    else {
        // Load key
        std::string key;
        ar(CEREAL_NVP_("key", key));

        // Create component instance
        p = lm::comp::create<T>(key);
        if (!p) {
            return;
        }

        // Load the contents
        p->load(ar);
    }
}

LM_NAMESPACE_END(cereal)

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(serial)

// ----------------------------------------------------------------------------

/*!
    \brief Serialize an object with given type.
*/
template <typename T>
void save(std::ostream& os, T&& v) {
    OutputArchive ar(os);
    ar(std::forward<T>(v));
}

/*!
    \brief Deserialize an object with given type.
*/
template <typename T>
void load(std::istream& is, T& v) {
    InputArchive ar(is);
    ar(v);
}

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(serial)
LM_NAMESPACE_END(LM_NAMESPACE)
