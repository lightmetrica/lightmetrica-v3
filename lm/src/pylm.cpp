/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>
#include <lm/pylm.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \brief Binds Lightmetrica to a Python module.
*/
static void bind(pybind11::module& m) {
    using namespace pybind11::literals;

    // ------------------------------------------------------------------------

    // component.h

    // Trampoline class for lm::Component
    class Component_Py final : public Component {
    public:
        virtual bool construct(const Json& prop) override {
            PYBIND11_OVERLOAD(bool, Component, construct, prop);
        }
    };
    pybind11::class_<Component, Component_Py, Component::Ptr<Component>>(m, "Component")
        .def(pybind11::init<>())
        .def("construct", &Component::construct)
        .def("parent", &Component::parent)
        .PYLM_DEF_COMP_BIND(Component);

    {
        // Namespaces are handled as submodules
        auto sm = m.def_submodule("comp");
        auto sm_detail = sm.def_submodule("detail");

        // Component API
        sm_detail.def("loadPlugin", &comp::detail::loadPlugin);
        sm_detail.def("loadPlugins", &comp::detail::loadPlugins);
        sm_detail.def("unloadPlugins", &comp::detail::unloadPlugins);
        sm_detail.def("foreachRegistered", &comp::detail::foreachRegistered);
        sm_detail.def("printRegistered", &comp::detail::printRegistered);
    }

    // ------------------------------------------------------------------------

    // user.h
    m.def("init", &init);
    m.def("shutdown", &shutdown);
    m.def("asset", &asset);
    m.def("primitive", &primitive);
    m.def("primitives", &primitives);
    m.def("render", &render);
    m.def("save", &save);
    m.def("buffer", &buffer);

    // ------------------------------------------------------------------------

    // logger.h
    {
        auto sm = m.def_submodule("log");

        // LogLevel
        pybind11::enum_<log::LogLevel>(sm, "LogLevel")
            .value("Debug", log::LogLevel::Debug)
            .value("Info", log::LogLevel::Info)
            .value("Warn", log::LogLevel::Warn)
            .value("Err", log::LogLevel::Err)
            .value("Progress", log::LogLevel::Progress)
            .value("ProgressEnd", log::LogLevel::ProgressEnd);

        // Log API
        sm.def("init", &log::init);
        sm.def("shutdown", &log::shutdown);
        using LogFuncPtr = void(*)(log::LogLevel, const char*, int, const char*);
        sm.def("log", (LogFuncPtr)&log::log);
        sm.def("updateIndentation", &log::updateIndentation);

        // Context
        using LoggerContext = log::detail::LoggerContext;
        class LoggerContext_Py final : public LoggerContext {
            virtual bool construct(const Json& prop) override {
                PYBIND11_OVERLOAD(bool, LoggerContext, construct, prop);
            }
            virtual void log(log::LogLevel level, const char* filename, int line, const char* message) override {
                PYBIND11_OVERLOAD_PURE(void, LoggerContext, log, level, filename, line, message);
            }
            virtual void updateIndentation(int n) override {
                PYBIND11_OVERLOAD_PURE(void, LoggerContext, updateIndentation, n);
            }
        };
        pybind11::class_<LoggerContext, LoggerContext_Py, Component::Ptr<LoggerContext>>(sm, "LoggerContext")
            .def(pybind11::init<>())
            .def("log", &LoggerContext::log)
            .def("updateIndentation", &LoggerContext::updateIndentation)
            .PYLM_DEF_COMP_BIND(LoggerContext);
    }

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
}

// ----------------------------------------------------------------------------

PYBIND11_MODULE(pylm, m) {
    m.doc() = R"x(
        pylm: Python binding of Lightmetrica.
    )x";
    bind(m);
}

LM_NAMESPACE_END(LM_NAMESPACE)
