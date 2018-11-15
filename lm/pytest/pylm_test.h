/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include <lm/pylm.h>
#include <lm/component.h>

#define LM_TEST_NAMESPACE lmtest

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

namespace py = pybind11;
using namespace py::literals;

/*!
    \brief Module binder for python tests.
*/
class PyTestBinder : public lm::Component {
public:
    /*!
        \brief Bind to the module.
    */
    virtual void bind(py::module& m) const = 0;
};

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
