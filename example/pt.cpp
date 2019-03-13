/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>

/*
    Example of rendering an image with path tracing explaining basic usage of user APIs.

    Example:
    $ ./004_pt ./scenes/fireplace_room/fireplace_room.obj result.pfm \
               10 20 1920 1080 \
               5.101118 1.083746 -2.756308 \
               4.167568 1.078925 -2.397892 \
               43.001194
*/
int main(int argc, char** argv) {
    try {
        // Initialize the framework
        lm::init();
        lm::parallel::init(lm::parallel::DefaultType, {
            #if LM_DEBUG_MODE
            {"numThreads", 1}
            #else
            {"numThreads", -1}
            #endif
        });
        lm::info();

        // Parse command line arguments
        const auto opt = lm::json::parsePositionalArgs<13>(argc, argv, R"({{
            "obj": "{}",
            "out": "{}",
            "spp": {},
            "len": {},
            "w": {},
            "h": {},
            "eye": [{},{},{}],
            "lookat": [{},{},{}],
            "vfov": {}
        }})");

        // --------------------------------------------------------------------

        #if LM_DEBUG_MODE
        // Load internal state
        lm::deserialize("lm.serialized");
        #else
        // Define assets

        // Film for the rendered image
        lm::asset("film1", "film::bitmap", {
            {"w", opt["w"]},
            {"h", opt["h"]}
        });

        // Pinhole camera
        lm::asset("camera1", "camera::pinhole", {
            {"film", lm::asset("film1")},
            {"position", opt["eye"]},
            {"center", opt["lookat"]},
            {"up", {0,1,0}},
            {"vfov", opt["vfov"]}
        });

        // OBJ model
        lm::asset("obj1", "model::wavefrontobj", { {"path", opt["obj"]} });

        // Define scene primitives

        // Camera
        lm::primitive(lm::Mat4(1), {
            {"camera", lm::asset("camera1")}
        });

        // Create primitives from model asset
        lm::primitive(lm::Mat4(1), {
            {"model", lm::asset("obj1")}
        });

        // Build acceleration structure
        lm::build("accel::sahbvh");

        // Save internal state for the debug mode
        lm::serialize("lm.serialized");
        #endif

        // --------------------------------------------------------------------

        // Render an image
        // _begin_render
        lm::render("renderer::pt", {
            {"output", lm::asset("film1")},
            {"spp", opt["spp"]},
            {"maxLength", opt["len"]}
        });
        // _end_render

        // Save rendered image
        lm::save(lm::asset("film1"), opt["out"]);

        // Shutdown the framework
        lm::shutdown();
    }
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }

    return 0;
}