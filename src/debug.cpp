/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/debug.h>
#include <lm/parallel.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE::debug)

class Context {
public:
    static Context& instance() {
        static Context instance;
        return instance;
    }

private:
    OnPollFloatFunc onPollFloat_;

public:
    void pollFloat(const std::string& name, Float val) {
        if (!parallel::mainThread()) {
            return;
        }
        if (!onPollFloat_) {
            throw std::runtime_error("onPollFloat function is not registered.");
        }
        onPollFloat_(name, val);
    }

    void regOnPollFloat(const OnPollFloatFunc& onPollFloat) {
        onPollFloat_ = onPollFloat;
    }
};

// ----------------------------------------------------------------------------

LM_PUBLIC_API void pollFloat(const std::string& name, Float val) {
    Context::instance().pollFloat(name, val);
}

LM_PUBLIC_API void regOnPollFloat(const OnPollFloatFunc& onPollFloat) {
    Context::instance().regOnPollFloat(onPollFloat);
}

LM_NAMESPACE_END(LM_NAMESPACE::debug)
