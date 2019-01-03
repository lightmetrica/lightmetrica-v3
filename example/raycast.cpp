/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>

/*
./003_raycast ./scenes/fireplace_room/fireplace_room.obj result.pfm \
              1920 1080 \
              5.101118 1.083746 -2.756308 \
              4.167568 1.078925 -2.397892 \
              43.001194
*/
int main(int argc, char** argv) {
    try {
        // Initialize the framework
        lm::init({
            #if LM_DEBUG_MODE
            {"numThreads", 1}
            #else
            {"numThreads", -1}
            #endif
        });

        // _begin_parse_args
        // Parse command line arguments
        const auto opt = lm::parsePositionalArgs<11>(argc, argv, R"({{
            "obj": "{}",
            "out": "{}",
            "w": {},
            "h": {},
            "eye": [{},{},{}],
            "lookat": [{},{},{}],
            "vfov": {}
        }})");
        // _end_parse_args

        // --------------------------------------------------------------------

        // Define assets

        // _begin_asset
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
        lm::asset("obj1", "model::wavefrontobj", {{"path", opt["obj"]}});
        // _end_asset

        // --------------------------------------------------------------------

        // Define scene primitives

        // _begin_primitives
        // Camera
        lm::primitive(lm::Mat4(1), {{"camera", "camera1"}});

        // Create primitives from model asset
        lm::primitives(lm::Mat4(1), "obj1");
        // _end_primitives

        // --------------------------------------------------------------------

        // Render an image
        lm::build("accel::sahbvh");
        lm::render("renderer::raycast", {
            {"output", "film1"},
            {"color", {0,0,0}}
        });

        // Save rendered image
        lm::save("film1", opt["out"]);
    }
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }

    return 0;
}