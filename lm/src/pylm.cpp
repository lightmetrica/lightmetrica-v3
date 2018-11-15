/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>
#include <lm/pylm.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// ----------------------------------------------------------------------------

// logger.h
#if 0
LM_NAMESPACE_BEGIN(log::detail)
class LoggerContext_Py : public LoggerContext {
    virtual bool construct(const Json& prop) override {
        
    }
    virtual void log(LogLevel level, const char* filename, int line, const char* message) override {

    }
    virtual void updateIndentation(int n) {
        
    }
};
LM_NAMESPACE_END(log::detail)
#endif

// ----------------------------------------------------------------------------

PYBIND11_MODULE(pylm, m)
{
    using namespace pybind11::literals;

    // Module description
    m.doc() = R"x(
        pylm: Python binding of Lightmetrica.
    )x";

    // Register component classes
    

    // ------------------------------------------------------------------------

    // Film buffer (film.h)
    pybind11::class_<FilmBuffer>(m, "FilmBuffer", pybind11::buffer_protocol())
        // Register buffer description
        .def_buffer([](FilmBuffer& buf) -> pybind11::buffer_info {
            return pybind11::buffer_info(
                buf.data,
                sizeof(Float),
                pybind11::format_descriptor<Float>::format(),
                3,
                { buf.h, buf.w, 3 },
                { 3 * buf.w * sizeof(Float),
                  3 * sizeof(Float),
                  sizeof(Float) }
            );
        });

    // ------------------------------------------------------------------------

    // Register context APIs
    // user.h
    m.def("init", &init);
    m.def("shutdown", &shutdown);
    m.def("asset", &asset);
    m.def("primitive", &primitive);
    m.def("primitives", &primitives);
    m.def("render", &render);
    m.def("save", &save);
    m.def("buffer", &buffer);

    // logger.h
    // Namespaces are handled as submodules
    {
        auto sm = m.def_submodule("log");
        sm.def("init", &log::init);
        sm.def("shutdown", &log::shutdown);
        sm.def("log", &log::log);
        sm.def("updateIndentation", &log::updateIndentation);
    }
}

LM_NAMESPACE_END(LM_NAMESPACE)
