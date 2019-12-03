/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

// _begin_include
#include <lm/lm.h>
// _end_include

int main(int argc, char** argv) {
    try {
        // _begin_init
        // Initialize the framework
        lm::init();
        lm::info();
        // _end_init

        // _begin_parse_command_line
        // Parse command line arguments
        const auto opt = lm::json::parse_positional_args<3>(argc, argv, R"({{
            "out": "{}",
            "w": {},
            "h": {}
        }})");
        // _end_parse_command_line

        // _begin_assets
        // Define assets
        // Film for the rendered image
        lm::asset("film", "film::bitmap", {
            {"w", opt["w"]},
            {"h", opt["h"]}
        });
        // _end_assets

        // _begin_render
        // Render an image
        lm::render("renderer::blank", {
            {"output", lm::asset("film")},
            {"color", {1,1,1}}
        });
        // _end_render

        // _begin_save
        // Save rendered image
        lm::save(lm::asset("film"), opt["out"]);
        // _end_save

        // _begin_shutdown
        // Shutdown the framework
        lm::shutdown();
        // _end_shutdown
    }
    // _begin_exception
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }
    // _end_exception

    return 0;
}