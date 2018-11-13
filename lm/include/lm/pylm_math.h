/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include "math.h"
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

LM_NAMESPACE_BEGIN(pybind11::detail)

// Type caster for glm::vec type
template <int N, typename T, glm::qualifier Q>
struct type_caster<glm::vec<N, T, Q>> {
    using VecT = glm::vec<N, T, Q>;

    // Register this class as type caster
    PYBIND11_TYPE_CASTER(VecT, _("glm::vec"));

    // Python -> C++
    bool load(handle src, bool convert) {
        if (!convert) {
            // No-convert mode
            return false;
        }

        if (!isinstance<array_t<T>>(src)) {
            // Invalid numpy array type
            return false;
        }

        // Ensure that the argument is a numpy array of correct type
        auto buf = array::ensure(src);
        if (!buf) {
            return false;
        }

        // Check dimension
        if (buf.ndim() != 1) {
            return false;
        }

        // Copy values
        memcpy(&value.data, buf.data(), N * sizeof(T));

        return true;
    }

    // C++ -> Python
    static handle cast(VecT&& src, return_value_policy policy, handle parent) {
        LM_UNUSED(policy);
        // Create numpy array from VecT
        array a(
            {N},          // Shapes
            {sizeof(T)},  // Strides
            &src.x,       // Data
            parent        // Parent handle
        );
        return a.release();
    }
};


// Type caster for glm::mat type
template <int C, int R, typename T, glm::qualifier Q>
struct type_caster<glm::mat<C, R, T, Q>> {
    using MatT = glm::mat<C, R, T, Q>;

    // Register this class as type caster
    PYBIND11_TYPE_CASTER(MatT, _("glm::mat"));

    // Python -> C++
    bool load(handle src, bool convert) {
        if (!convert) {
            // No-convert mode
            return false;
        }

        if (!isinstance<array_t<T>>(src)) {
            // Invalid numpy array type
            return false;
        }

        // Ensure that the argument is a numpy array of correct type
        auto buf = array::ensure(src);
        if (!buf) {
            return false;
        }

        // Check dimension
        if (buf.ndim() != 2) {
            return false;
        }

        // Copy values
        memcpy(&value[0].data, buf.data(), C * R * sizeof(T));

        // Transpose the copied values because
        // glm is column major on the other hand numpy is row major.
        glm::transpose(value);

        return true;
    }

    // C++ -> Python
    static handle cast(MatT&& src, return_value_policy policy, handle parent) {
        LM_UNUSED(policy);
        // Create numpy array from MatT
        src = glm::transpose(src);
        array a(
            {R, C},                    // Shapes
            {R*sizeof(T), sizeof(T)},  // Strides
            &src[0].x,                 // Data
            parent                     // Parent handle
        );
        return a.release();
    }

};

LM_NAMESPACE_END(pybind11::detail)
