/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include "test_common.h"
#include <generated/test_python.h>
#include <pybind11/embed.h>
#include <lm/detail/pylm.h>

namespace py = pybind11;
using namespace py::literals;

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

PYBIND11_EMBEDDED_MODULE(test_pylm, m) {
    m.def("dump", [](lm::json v) -> void {
        std::cout << v.dump();
    });
    m.def("round_trip", [](lm::json v) -> lm::json {
        return v;
    });
}

TEST_CASE("Casting JSON type") {
    Py_SetPythonHome(LM_TEST_PYTHON_ROOT);
    py::scoped_interpreter guard{};

    try {
        py::exec(R"(
            import test_pylm
        )", py::globals());

        SUBCASE("Convert argument") {
            SUBCASE("bool 1") {
                const auto out = captureStdout([]() {
                    py::exec(R"(
                        test_pylm.dump(True)
                    )", py::globals());
                });
                CHECK(out == "true");
            }
            SUBCASE("bool 2") {
                const auto out = captureStdout([]() {
                    py::exec(R"(
                        test_pylm.dump(False)
                    )", py::globals());
                });
                CHECK(out == "false");
            }
        }
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        REQUIRE(false);
    }
}

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
