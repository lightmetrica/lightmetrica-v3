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
            {"debugio", "debugio::server"}
        });

        
    }
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }

    return 0;
}