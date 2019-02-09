/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>

/*
    This example illustrates how to integrate debugio client to your renderer.
*/
int main(int argc, char** argv) {
    try {
        // Initialize the framework
        lm::init("user::default", {
            {"numThreads", 1},
            {"debugio", {
                {"debugio::client", {
                    {"address", "tcp://localhost:5555"}
                }}
            }}
        });

        // Parse command line arguments
        const auto opt = lm::json::parsePositionalArgs<11>(argc, argv, R"({{
            "obj": "{}",
            "out": "{}",
            "w": {},
            "h": {},
            "eye": [{},{},{}],
            "lookat": [{},{},{}],
            "vfov": {}
        }})");

        // --------------------------------------------------------------------

        // Define assets and primitives
        #if LM_DEBUG_MODE
        lm::deserialize("lm.serialized");
        #else
        lm::asset("film_render", "film::bitmap", {
            {"w", opt["w"]},
            {"h", opt["h"]}
        });
        lm::asset("camera_render", "camera::pinhole", {
            {"film", "film_render"},
            {"position", opt["eye"]},
            {"center", opt["lookat"]},
            {"up", {0,1,0}},
            {"vfov", opt["vfov"]}
        });
        lm::asset("obj1", "model::wavefrontobj", {
            {"path", opt["obj"]}
        });
        lm::primitive(lm::Mat4(1), {{"camera", "camera_render"}});
        lm::primitives(lm::Mat4(1), "obj1");
        lm::build("accel::sahbvh");
        lm::serialize("lm.serialized");
        #endif

        // --------------------------------------------------------------------

        lm::debugio::syncUserContext();

        // --------------------------------------------------------------------

        // Render an image
        //lm::render("renderer::raycast", {
        //    {"output", "film1"},
        //    {"color", {0,0,0}}
        //});

        // Save rendered image
        //lm::save("film1", opt["out"]);

        // Shutdown the framework
        lm::shutdown();
    }
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }

    return 0;
}