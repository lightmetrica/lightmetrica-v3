#include <lm/pylm.h>
#include "tuner.h"
#include <iostream>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

PYBIND11_MODULE(py_tuner, m) {
    m.doc() = "Python binding for tuner";
    std::cout<<"executed"<<std::endl;
    // Bind free function
    //m.def("test_function", &test_function);
    
    // Define a trampoline class for the interface
    // ref. https://pybind11.readthedocs.io/en/stable/advanced/classes.html
    class TunerComponent_Py final : public Tuner {
        virtual void construct(const lm::Json& prop) override {
            PYBIND11_OVERLOAD(void, Tuner, construct, prop);
        }
        virtual Json feedback(Float fb) override {
            PYBIND11_OVERLOAD_PURE(Json, Tuner, feedback, fb);
        }
        virtual Json getConf() override {
            PYBIND11_OVERLOAD_PURE(Json, Tuner, getConf);
        }
    };
    std::cout<<"executed"<<std::endl;
    // You must not add .def() for construct() function
    // which is already registered in parent class.
    pybind11::class_<Tuner, TunerComponent_Py, Component::Ptr<Tuner>>(m, "Tuner")
        .def(pybind11::init<>())
        .def("feedback", &Tuner::feedback)
        .def("getConf", &Tuner::getConf)
        .PYLM_DEF_COMP_BIND(Tuner);
    std::cout<<"executed"<<std::endl;
}



LM_NAMESPACE_END(LM_NAMESPACE)