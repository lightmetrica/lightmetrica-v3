/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>


/*
    This example illustrates interactive visualization support.
*/
int main(int argc, char** argv) {
    try {
        // Initialize the framework
        lm::init();

        // We need `lmgl` plugin to use interactive features
        lm::comp::detail::ScopedLoadPlugin pluginGuard("lmgl");

        // Parse command line arguments
        const auto opt = lm::json::parsePositionalArgs<13>(argc, argv, R"({{
            "obj": "{}",
            "out": "{}",
            "spp": {},
            "w": {},
            "h": {},
            "eye": [{},{},{}],
            "lookat": [{},{},{}],
            "vfov": {}
        }})");

        // --------------------------------------------------------------------

        // Define assets

        // Film for the rendered image
        lm::asset("film1", "film::bitmap", {
            {"w", opt["w"]},
            {"h", opt["h"]}
        });

        // Pinhole camera
        lm::asset("camera1", "camera::pinhole", {
            {"film", "film1"},
            {"position", opt["eye"]},
            {"center", opt["lookat"]},
            {"up", {0,1,0}},
            {"vfov", opt["vfov"]}
        });

        // OBJ model
        // Replace all materials to diffuse and use our checker texture
        lm::asset("obj1", "model::wavefrontobj", {
            {"path", opt["obj"]}
        });

        // --------------------------------------------------------------------

        // Define scene primitives

        // Camera
        lm::primitive(lm::Mat4(1), { {"camera", "camera1"} });

        // Create primitives from model asset
        lm::primitives(lm::Mat4(1), "obj1");

        // --------------------------------------------------------------------

        // Render an image
        lm::build("accel::sahbvh");
        lm::render("renderer::ao2", {
            {"output", "film1"},
            {"spp", opt["spp"]}
        });

        // Save rendered image
        lm::save("film1", opt["out"]);

        // Shutdown the framework
        lm::shutdown();
    }
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }

    return 0;
}
