/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include "component.h"
#include "math.h"
#include "logger.h"
#include "ext/cereal/types/string.hpp"
#include "ext/cereal/types/vector.hpp"
#include "ext/cereal/types/unordered_map.hpp"
#include <atomic>

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(cereal)

/*
    Serialize function specialized for std::optional.
*/
template <class Archive, typename T>
void save(Archive& ar, const std::optional<T>& optional) {
    if (!optional) {
        ar(CEREAL_NVP_("nullopt", true));
    }
    else {
        ar(CEREAL_NVP_("nullopt", false), CEREAL_NVP_("data", *optional));
    }
}

/*
    Serialize function specialized for std::optional.
*/
template <class Archive, typename T>
void load(Archive& ar, std::optional<T>& optional) {
    bool nullopt;
    ar(CEREAL_NVP_("nullopt", nullopt));
    if (nullopt) {
        optional = std::nullopt;
    }
    else {
        T value;
        ar(CEREAL_NVP_("data", value));
        optional = std::move(value);
    }
}

// ----------------------------------------------------------------------------

/*
    Serialize function specialized for glm::vec<>.
*/
template <typename Archive, int N, typename T, glm::qualifier Q>
void serialize(Archive& ar, glm::vec<N, T, Q>& v) {
    for (int i = 0; i < N; i++) {
        ar(v[i]);
    }
}

/*
    Serialize function specialized for glm::mat<>.
*/
template <typename Archive, int C, int R, typename T, glm::qualifier Q>
void serialize(Archive& ar, glm::mat<C, R, T, Q>& v) {
    for (int i = 0; i < C; i++) {
        ar(v[i]);
    }
}

// ----------------------------------------------------------------------------

/*
    Save function specialized for std::atomic<T>.
*/
template <typename Archive, typename T>
void save(Archive& ar, const std::atomic<T>& v) {
    T t = v;
    ar(t);
}

/*
    Load function specialized for std::atomic<T>.
*/
template <class Archive, class T>
void load(Archive& ar, std::atomic<T>& v) {
    T t;
    ar(t);
    v = t;
}

// ----------------------------------------------------------------------------


/*
    Save owned pointer.
*/
template <typename Archive, typename T>
void saveOwned(Archive& ar, T* p) {
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
        ar(CEREAL_NVP_("key", Access::key(p)));

        // Consistency testing checking if the locator is valid
        const auto& loc = Access::loc(p);
        if (!loc.empty()) {
            const auto* p_loc = lm::comp::get<T>(loc);
            if (!p_loc || p_loc != p) {
                LM_ERROR("Invalid locator [loc='{}']", loc);
                LM_ERROR("Loaded state might be broken. Check if");
                LM_ERROR("  - locator is properly specified in lm::comp::create()");
                LM_ERROR("  - underlying() function is properly implemented");
            }
        }
        ar(CEREAL_NVP_("loc", loc));

        // Save the contants with Component::save() function.
        // We don't use cereal's polymorphinc class support
        // because their features can be achievable with our component system.
        p->save(ar);
    }
}

/*
    Save function specialized for Component::Ptr<T>.
*/
template <typename Archive, typename T>
void save(Archive& ar, const lm::Component::Ptr<T>& p) {
    saveOwned(ar, p.get());
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

// ----------------------------------------------------------------------------

/*
    Specialization of serialize() function for Component*.

    cereal library does not support specialization for pointer types.
    We relax this requirement to support T* if T is or inherited Component class.
    However, cereal library specializes the serialize() function
    to generate error message for the unsupported T* types.
    To supress this unexpected error message, we specialize the function here.
*/

// Overload for OutputArchive corresponding save()
template <typename T>
std::enable_if_t<
    std::is_base_of_v<lm::Component, T>,
    void
>
serialize(lm::OutputArchive& ar, T*& p) {
    using Archive = lm::OutputArchive;
    using Access = lm::comp::detail::Access;
    if (!p) {
        ar(CEREAL_NVP_("valid", uint8_t(0)));
    }
    else {
        ar(CEREAL_NVP_("valid", uint8_t(1)));
        ar(CEREAL_NVP_("loc", Access::loc(p)));
        if (Access::loc(p).empty()) {
            LM_ERROR("Serializing weak reference requires global locator [key='{}']", Access::key(p));
        }
    }
}

// Overload for InputArchive corresponding load()
template <typename T>
std::enable_if_t<
    std::is_base_of_v<lm::Component, T>,
    void
>
serialize(lm::InputArchive& ar, T*& p) {
    using Archive = lm::InputArchive;
    uint8_t valid;
    ar(CEREAL_NVP_("valid", valid));
    if (!valid) {
        p = nullptr;
    }
    else {
        std::string loc;
        ar(CEREAL_NVP_("loc", loc));
        p = lm::comp::get<T>(loc);
        if (!p) {
            LM_ERROR("Invalid global locator [locator='{}']", loc);
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
template <typename... Ts>
void save(std::ostream& os, Ts&&... v) {
    OutputArchive ar(os);
    ar(std::forward<Ts>(v)...);
}

/*!
    \brief Serialize a component as owned pointer.
*/
LM_INLINE void saveOwned(std::ostream& os, Component* v) {
    OutputArchive ar(os);
    cereal::saveOwned(ar, v);
}

/*!
    \brief Deserialize an object with given type.
*/
template <typename... Ts>
void load(std::istream& is, Ts&... v) {
    InputArchive ar(is);
    ar(v...);
}

/*!
    \brief Serialize an object to a file.
*/
template <typename T>
void save(const std::string& path, T&& v) {
    std::ofstream os(path, std::ios::out | std::ios::binary);
    save(os, std::forward<T>(v));
}

/*!
    \brief Deserialize an object from an file.
*/
template <typename T>
void load(const std::string& path, T& v) {
    std::ifstream is(path, std::ios::in | std::ios::binary);
    load(is, v);
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