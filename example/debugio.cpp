/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>

/*
    Illustrates the usage of debugio subsystem as a client.
*/
int main() {
    try {
        // Initialize the framework with debugio client
        lm::init("user::default", {
            {"debugio", {
                {"debugio::client", {
                    {"address", "tcp://localhost:5555"}
                }}
            }}
        });

        // Send a message to the server
        lm::debugio::handleMessage("hai domo");

        // Synchronize the state of user context with server
        lm::debugio::syncUserContext();
        
        // Debugio supports API of visual debugging
        lm::debugio::draw(0, {}, {});

        // Shutdown the framework
        lm::shutdown();
    }
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }

    return 0;
}