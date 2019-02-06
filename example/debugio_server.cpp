/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>

class MyDebugioContext : public lm::debugio::DebugioContext {
public:
    virtual void handleMessage(const std::string& message) override {
        LM_INFO("handleMessage");
        LM_INFO(message);
    }

    virtual void syncUserContext() override {
        LM_INFO("syncUserContext");
    }

    virtual void drawScene() override {
        LM_INFO("drawScene");
    }

    virtual void drawLineStrip(const std::vector<lm::Vec3>&) override {
        LM_INFO("drawLineStrip");
    }

    virtual void drawTriangles(const std::vector<lm::Vec3>&) override {
        LM_INFO("drawTriangles");
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

        // Start to run the server
        lm::debugio::run();

        // Shutdown the framework
        lm::shutdown();
    }
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }

    return 0;
}