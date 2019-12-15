/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "common.h"
#include <cereal/archives/portable_binary.hpp>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup serial
    @{
*/

/*!
    \brief Default output archive used by lm::Component.

    \rst
    We use cereal's portal binary archive as a default output archive type.
    This type is used as an argument type of :cpp:func:`lm::Component::save` function.
    \endrst
*/
class OutputArchive final : public cereal::OutputArchive<OutputArchive, cereal::AllowEmptyClassElision> {
private:
    // Locator of the root component
    std::string root_loc_;
    cereal::PortableBinaryOutputArchive archive_; 

public:
    OutputArchive(std::ostream& stream)
        : OutputArchive(stream, "")
    {}

    OutputArchive(std::ostream& stream, const std::string& root_loc)
        : cereal::OutputArchive<OutputArchive, cereal::AllowEmptyClassElision>(this)
        , archive_(stream)
        , root_loc_(root_loc)
    {}

    template <std::size_t DataSize> inline
    void saveBinary(const void* data, std::size_t size) {
        archive_.saveBinary<DataSize>(data, size);
    }

    std::string root_loc() const {
        return root_loc_;
    }
};

/*!
    \brief Default input archive used by lm::Component.

    \rst
    We use cereal's portal binary archive as a default input archive type.
    This type is used as an argument type of :cpp:func:`lm::Component::load` function.
    \endrst
*/
class InputArchive final : public cereal::InputArchive<InputArchive, cereal::AllowEmptyClassElision> {
private:
    // Locator of the root component
    std::string root_loc_;
    cereal::PortableBinaryInputArchive archive_;

    // Pairs of weak pointer and locators to be recovered later
    struct WeakptrAddressLocPair {
        std::uintptr_t address;
        std::string loc;
    };
    std::vector<WeakptrAddressLocPair> weakptr_loc_pairs_;

public:
    InputArchive(std::istream& stream)
        : InputArchive(stream, "")
    {}

    InputArchive(std::istream& stream, const std::string& root_loc)
        : cereal::InputArchive<InputArchive, cereal::AllowEmptyClassElision>(this)
        , archive_(stream)
        , root_loc_(root_loc)
    {}

    template <std::size_t DataSize> inline
    void loadBinary(void* const data, std::size_t size) {
        archive_.loadBinary<DataSize>(data, size);
    }

public:
    // Get root locator
    std::string root_loc() const {
        return root_loc_;
    }

    // Add a weak pointer entry to be updated later
    void add_weakptr(std::uintptr_t address, const std::string loc) {
        weakptr_loc_pairs_.push_back({ address, loc });
    }

    // Iterate over added weak pointers
    using ForeachWeakptrFunc = std::function<void(std::uintptr_t address, const std::string& loc)>;
    void foreach_weakptr(const ForeachWeakptrFunc& func) {
        for (auto p : weakptr_loc_pairs_) {
            func(p.address, p.loc);
        }
    }
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)

//! \cond

LM_NAMESPACE_BEGIN(cereal)

template<class T> inline
typename std::enable_if<std::is_arithmetic<T>::value, void>::type
CEREAL_SAVE_FUNCTION_NAME(LM_NAMESPACE::OutputArchive& ar, T const& t) {
    static_assert(
        !std::is_floating_point<T>::value ||
        (std::is_floating_point<T>::value && std::numeric_limits<T>::is_iec559),
        "Portable binary only supports IEEE 754 standardized floating point");
    ar.template saveBinary<sizeof(T)>(std::addressof(t), sizeof(t));
}

template<class T> inline
typename std::enable_if<std::is_arithmetic<T>::value, void>::type
CEREAL_LOAD_FUNCTION_NAME(LM_NAMESPACE::InputArchive& ar, T& t) {
    static_assert(
        !std::is_floating_point<T>::value ||
        (std::is_floating_point<T>::value && std::numeric_limits<T>::is_iec559),
        "Portable binary only supports IEEE 754 standardized floating point");
    ar.template loadBinary<sizeof(T)>(std::addressof(t), sizeof(t));
}

template <class Archive, class T> inline
CEREAL_ARCHIVE_RESTRICT(LM_NAMESPACE::InputArchive, LM_NAMESPACE::OutputArchive)
CEREAL_SERIALIZE_FUNCTION_NAME(Archive& ar, cereal::NameValuePair<T>& t) {
    ar(t.value);
}

template <class Archive, class T> inline
CEREAL_ARCHIVE_RESTRICT(LM_NAMESPACE::InputArchive, LM_NAMESPACE::OutputArchive)
CEREAL_SERIALIZE_FUNCTION_NAME(Archive& ar, cereal::SizeTag<T>& t) {
    ar(t.size);
}

template <class T> inline
void CEREAL_SAVE_FUNCTION_NAME(LM_NAMESPACE::OutputArchive& ar, cereal::BinaryData<T> const& bd) {
    typedef typename std::remove_pointer<T>::type TT;
    static_assert(!std::is_floating_point<TT>::value ||
        (std::is_floating_point<TT>::value && std::numeric_limits<TT>::is_iec559),
        "Portable binary only supports IEEE 754 standardized floating point");

    ar.template saveBinary<sizeof(TT)>(bd.data, static_cast<std::size_t>(bd.size));
}

//! Loading binary data from portable binary
template <class T> inline
void CEREAL_LOAD_FUNCTION_NAME(LM_NAMESPACE::InputArchive& ar, cereal::BinaryData<T>& bd) {
    typedef typename std::remove_pointer<T>::type TT;
    static_assert(!std::is_floating_point<TT>::value ||
        (std::is_floating_point<TT>::value && std::numeric_limits<TT>::is_iec559),
        "Portable binary only supports IEEE 754 standardized floating point");

    ar.template loadBinary<sizeof(TT)>(bd.data, static_cast<std::size_t>(bd.size));
}

LM_NAMESPACE_END(cereal)

CEREAL_REGISTER_ARCHIVE(LM_NAMESPACE::OutputArchive)
CEREAL_REGISTER_ARCHIVE(LM_NAMESPACE::InputArchive)
CEREAL_SETUP_ARCHIVE_TRAITS(LM_NAMESPACE::InputArchive, LM_NAMESPACE::OutputArchive)

//! \endcond
