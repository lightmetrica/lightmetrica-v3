/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>
#include <lm/pylm.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

PYBIND11_MODULE(pylm, m)
{
    m.doc() = R"x(
        pylm: Python binding of Lightmetrica.
    )x";

    // ------------------------------------------------------------------------

    // User API (user.h)
    m.def("init", &init);
    m.def("shutdown", &shutdown);
    m.def("asset", &asset);
    m.def("primitive", &primitive);
    m.def("primitives", &primitives);
    m.def("render", &render);
    m.def("save", &save);

}

LM_NAMESPACE_END(LM_NAMESPACE)
