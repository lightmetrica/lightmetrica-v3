/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>

class MyDebugioContext : public lm::debugio::DebugioContext {
public:
    virtual void handleMessage(const std::string& message) override {
        LM_INFO(message);
    }
};

LM_COMP_REG_IMPL(MyDebugioContext, "debugio::mycontext");

/*
    Illustrates the usage of debugio subsystem as a server.
*/
int main() {
    try {
        // Initialize the framework with debugio server
        lm::init("user::default", {
            {"debugio", {
                {"debugio::server", {
                    {"port", 5555},
                    {"ref", "debugio::mycontext"}
                }}
            }}
        });

        lm::debugio::run();
    }
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }

    return 0;
}