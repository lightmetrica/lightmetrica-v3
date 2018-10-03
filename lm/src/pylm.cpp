/*
    Lightmetrica V3 Prototype
    Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/detail/pylm.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

PYBIND11_MODULE(pylm, m)
{
    m.doc() = R"x(
        pylm: Python binding of Lightmetrica.
    )x";

    m.def("primitive", &lm::api::primitive, R"x(

        Example:

        >>> pylm.primitive('test')
        42

    )x", pybind11::arg("name"));
}

LM_NAMESPACE_END(LM_NAMESPACE)
