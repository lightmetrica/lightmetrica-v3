/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>
#include <lm/pylm.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

PYBIND11_MODULE(pylm, m)
{
    using namespace pybind11::literals;

    // ------------------------------------------------------------------------

    m.doc() = R"x(
        pylm: Python binding of Lightmetrica.
    )x";

    // ------------------------------------------------------------------------

#if 0
    // Math types
    // Vec3
    pybind11::class_<Vec3>(m, "Vec3", pybind11::buffer_protocol())
        // Construction from buffer
        .def(pybind11::init([](pybind11::buffer b) {
            // Buffer description
            const auto info = b.request();
            // Check types
            if (info.format != pybind11::format_descriptor<Float>::format()) {
                throw std::runtime_error("Invalid buffer format");
            }
            if (info.ndim != 1) {
                throw std::runtime_error("Invalid buffer dimension");
            }
            // Create vector instance
            std::unique_ptr<Vec3> v(new Vec3);
            memcpy(&v->data, info.ptr, sizeof(Float) * 3);
            return v.release();
        }))
        // Register buffer description
        .def_buffer([](Vec3& v) -> pybind11::buffer_info {
            return pybind11::buffer_info(
                &v.data,
                sizeof(Float),
                pybind11::format_descriptor<Float>::format(),
                1,
                { 3 },
                { 3 * sizeof(Float) }
            );
        });
#endif

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
