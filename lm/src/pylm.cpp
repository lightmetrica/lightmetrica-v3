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

PYBIND11_MODULE(pylm, m) {
    m.doc() = R"x(
        pylm: Python binding of Lightmetrica.
    )x";
    py::init(m);
}

LM_NAMESPACE_END(LM_NAMESPACE)
