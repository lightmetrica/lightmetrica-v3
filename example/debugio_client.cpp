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

        lm::debugio::handleMessage("hai domo");
    }
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }

    return 0;
}