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
            {"film", lm::asset("film_render")},
            {"position", opt["eye"]},
            {"center", opt["lookat"]},
            {"up", {0,1,0}},
            {"vfov", opt["vfov"]}
        });
        lm::asset("obj1", "model::wavefrontobj", {
            {"path", opt["obj"]}
        });
        lm::primitive(lm::Mat4(1), {
            {"camera", lm::asset("camera_render")}
        });
        lm::primitive(lm::Mat4(1), {
            {"model", lm::asset("obj1")}
        });
        lm::build("accel::sahbvh");
        lm::serialize("lm.serialized");
        #endif

        // --------------------------------------------------------------------

        lm::debugio::syncUserContext();

        lm::debugio::draw(lm::debugio::LineStrip, lm::Vec3(1), {
            lm::Vec3(0,0,0),
            lm::Vec3(0,0,10),
            lm::Vec3(0,10,0)
        });
        lm::debugio::draw(lm::debugio::Triangles, lm::Vec3(1), {
            lm::Vec3(1,0,0),
            lm::Vec3(1,0,10),
            lm::Vec3(1,10,0)
        });
        lm::debugio::draw(lm::debugio::Points, lm::Vec3(1), {
            lm::Vec3(2,0,0),
            lm::Vec3(2,0,10),
            lm::Vec3(2,10,0)
        });

        // --------------------------------------------------------------------

        // Shutdown the framework
        lm::shutdown();
    }
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }

    return 0;
}