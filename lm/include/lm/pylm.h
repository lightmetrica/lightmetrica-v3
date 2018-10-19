/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "detail/pylm_component.h"
#include "detail/pylm_json.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(py)

/*!
    \brief Binds Lightmetrica's component interfaces to the specified module.
*/
static void bindInterfaces(pybind11::module& m) {
    detail::Component_Py::bind(m);
}

LM_NAMESPACE_END(py)
LM_NAMESPACE_END(LM_NAMESPACE)
