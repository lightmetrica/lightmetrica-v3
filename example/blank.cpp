/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

// _begin_example
#include <lm/lm.h>
#include <filesystem>

int main(int argc, char** argv) {
    try {
        // Initialize the framework
        lm::init();

        // Parse command line arguments
        const auto opt = lm::json::parsePositionalArgs<3>(argc, argv, R"({{
            "out": "{}",
            "w": {},
            "h": {}
        }})");

        // Define assets
        // Film for the rendered image
        lm::asset("film", "film::bitmap", {{"w", opt["w"]}, {"h", opt["h"]}});

        // Render an image
        lm::render("renderer::blank", {
            {"output", "film"},
            {"color", {1,1,1}}
        });

        // Save rendered image
        lm::save("film", opt["out"]);
    }
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }

    return 0;
}
// _end_example