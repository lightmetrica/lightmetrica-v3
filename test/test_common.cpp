/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include "test_common.h"
#include <iostream>
#include <sstream>

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

std::string capture_stdout(const std::function<void()>& testFunc) {
    std::stringstream ss;
    auto* old = std::cout.rdbuf(ss.rdbuf());
    testFunc();
    const auto text = ss.str();
    std::cout.rdbuf(old);
    return text;
}

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
