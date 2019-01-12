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

/*
    Save function specialized for Component::Ptr<T>.
*/
template <typename Archive, typename T>
void save(Archive& ar, const lm::Component::Ptr<T>& p) {
    using Access = lm::comp::detail::Access;

    // Save an additional information if the pointer is nullptr
    if (!p) {
        // The pointer is invalid
        ar(CEREAL_NVP_("valid", uint8_t(0)));
    }
    else {
        // The pointer is valid
        ar(CEREAL_NVP_("valid", uint8_t(1)));

        // Meta information needed to recreate the instance
        ar(CEREAL_NVP_("key", Access::key(p.get())));
        ar(CEREAL_NVP_("loc", Access::loc(p.get())));

        // Save the contants with Component::save() function.
        // We don't use cereal's polymorphinc class support
        // because their features can be achievable with our component system.
        p->save(ar);
    }
}

/*
    Load function specialized for Component::Ptr<T>.
*/
template <typename Archive, typename T>
void load(Archive& ar, lm::Component::Ptr<T>& p) {
    uint8_t valid;
    ar(CEREAL_NVP_("valid", valid));
    if (!valid) {
        p.reset();
    }
    else {
        // Load key and locator
        std::string key;
        ar(CEREAL_NVP_("key", key));
        std::string loc;
        ar(CEREAL_NVP_("loc", loc));

        // Create component instance
        p = lm::comp::create<T>(key, loc);
        if (!p) {
            return;
        }

        // Load the contents
        p->load(ar);
    }
}

/*
    Specialization of serialize() function for Component*.

    cereal library does not support specialization for pointer types.
    We relax this requirement to support T* if T is or inherited Component class.
    However, cereal library specializes the serialize() function
    to generate error message for the unsupported T* types.
    To supress this unexpected error message, we specialize the function here.
*/
template <typename Archive>
void serialize(Archive& ar, lm::Component*& p) {
    LM_UNUSED(ar, p);
    if constexpr (std::is_same_v<Archive, lm::OutputArchive>) {
        // save
        using Access = lm::comp::detail::Access;
        if (!p) {
            ar(CEREAL_NVP_("valid", uint8_t(0)));
        }
        else {
            ar(CEREAL_NVP_("valid", uint8_t(1)));
            ar(CEREAL_NVP_("loc", Access::loc(p)));
        }
    }
    if constexpr (std::is_same_v<Archive, lm::InputArchive>) {
        // load
        uint8_t valid;
        ar(CEREAL_NVP_("valid", valid));
        if (!valid) {
            p = nullptr;
        }
        else {
            std::string loc;
            ar(CEREAL_NVP_("loc", loc));
            p = lm::comp::get<lm::Component>(loc);
        }
    }
}

LM_NAMESPACE_END(cereal)

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(serial)

// ----------------------------------------------------------------------------

/*!
    \addtogroup serial
    @{
*/

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

/*!
    @}
*/

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(serial)
LM_NAMESPACE_END(LM_NAMESPACE)

// ----------------------------------------------------------------------------

/*!
    \addtogroup serial
    @{
*/

/*!
    \brief Implement serialization function.
    \param ar Name of archive.
    
    \rst
    This function helps to implement load and save functions in your component class.
    Our serialization system depends on cereal library where it has input/output archives
    that can be usable with a same interface. So most of the simple types
    the user do not need to implement load and save functions separately
    and cereal provides a way to implement both fuction in the same time.
    This macro imitates this behavior. The user wants to give a function scope
    containing a serialization code and this macro automatically creates both functions.
    \endrst
*/
#define LM_SERIALIZE_IMPL(ar) \
    virtual void load(lm::InputArchive& ar_) override { serialize_(ar_); } \
    virtual void save(lm::OutputArchive& ar_) override { serialize_(ar_); } \
    template <typename Archive> void serialize_(Archive& ar)

/*!
    @}
*/