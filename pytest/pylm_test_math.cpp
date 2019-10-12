/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch_pylm.h>
#include <lm/pylm.h>

namespace py = pybind11;
using namespace py::literals;

LM_NAMESPACE_BEGIN(lmtest)

class PyTestBinder_Math : public lm::PyBinder {
public:
    virtual void bind(py::module& m) const {
        // Python -> C++
        m.def("compSum2", [](lm::Vec2 v) -> lm::Float {
            return v.x + v.y;
        });
        m.def("compSum3", [](lm::Vec3 v) -> lm::Float {
            return v.x + v.y + v.z;
        });
        m.def("compSum4", [](lm::Vec4 v) -> lm::Float {
            return v.x + v.y + v.z + v.w;
        });
        m.def("compMat4", [](lm::Mat4 m) -> lm::Float {
            lm::Float sum = 0;
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    sum += m[i][j];
                }
            }
            return sum;
        });

        // C++ -> Python
        m.def("getVec2", []() -> lm::Vec2 {
            return lm::Vec2(1, 2);
        });
        m.def("getVec3", []() -> lm::Vec3 {
            return lm::Vec3(1, 2, 3);
        });
        m.def("getVec4", []() -> lm::Vec4 {
            return lm::Vec4(1, 2, 3, 4);
        });
        m.def("getMat4", []() -> lm::Mat4 {
            return lm::Mat4(
                1,1,0,1,
                1,1,1,1,
                0,1,1,0,
                1,0,1,1
            );
        });
    }
};

LM_COMP_REG_IMPL(PyTestBinder_Math, "pytestbinder::math");

LM_NAMESPACE_END(lmtest)
