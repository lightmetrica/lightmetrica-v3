/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include "component.h"
#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/archives/portable_binary.hpp>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(serial)

//! Default input archive deligated to cereal library.
using InputArchive = cereal::PortableBinaryInputArchive;
//! Default output archive deligated to cereal library.
using OutputArchive = cereal::PortableBinaryOutputArchive;

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

LM_NAMESPACE_END(serial)
LM_NAMESPACE_END(LM_NAMESPACE)
