/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>

/*
    Illustrates the usage of debugio subsystem as a server.
*/
int main() {
    try {
        // Initialize the framework with debugio server
        lm::init("user::default", {
            {"debugio", {
                {"debugio::server", {
                    {"port", 5555}
                }}
            }}
        });

        // Register callback functions
        lm::debugio::server::on_handleMessage([](const std::string& message) {
            LM_INFO("handleMessage");
            LM_INFO(message);
        });
        lm::debugio::server::on_syncUserContext([]() {
            LM_INFO("syncUserContext");
        });
        lm::debugio::server::on_draw([](int, const std::vector<lm::Vec3>&) {
            LM_INFO("draw");
        });

        // Start to run the server
        lm::debugio::server::run();

        // Shutdown the framework
        lm::shutdown();
    }
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }

    return 0;
}