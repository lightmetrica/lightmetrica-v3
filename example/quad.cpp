/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>

int main(int argc, char** argv) {
    try {
        // Initialize the framework
        // _begin_init
        lm::init("user::default", {
            {"numThreads", -1}
        });
        // _end_init

        // Parse command line arguments
        const auto opt = lm::json::parsePositionalArgs<3>(argc, argv, R"({{
            "out": "{}",
            "w": {},
            "h": {}
        }})");

        // --------------------------------------------------------------------

        // Define assets

        // _begin_assets
        // Film for the rendered image
        lm::asset("film1", "film::bitmap", {{"w", opt["w"]}, {"h", opt["h"]}});

        // Pinhole camera
        lm::asset("camera1", "camera::pinhole", {
            {"film", "film1"},
            {"position", {0,0,5}},
            {"center", {0,0,0}},
            {"up", {0,1,0}},
            {"vfov", 30}
        });

        // Load mesh with raw vertex data
        lm::asset("mesh1", "mesh::raw", {
            {"ps", {-1,-1,-1,1,-1,-1,1,1,-1,-1,1,-1}},
            {"ns", {0,0,1}},
            {"ts", {0,0,1,0,1,1,0,1}},
            {"fs", {
                {"p", {0,1,2,0,2,3}},
                {"n", {0,0,0,0,0,0}},
                {"t", {0,1,2,0,2,3}}
            }}
        });

        // Material
        lm::asset("material1", "material::diffuse", {
            {"Kd", {1,1,1}}
        });
        // _end_assets

        // --------------------------------------------------------------------

        // Define scene primitives

        // _begin_primitive
        // Camera
        lm::primitive(lm::Mat4(1), {{"camera", "camera1"}});

        // Mesh
        lm::primitive(lm::Mat4(1), {
            {"mesh", "mesh1"},
            {"material", "material1"}
        });
        // _end_primitive

        // --------------------------------------------------------------------

        // Render an image
        // _begin_render
        lm::build("accel::sahbvh");
        lm::render("renderer::raycast", {
            {"output", "film1"},
            {"color", {0,0,0}}
        });
        // _end_render

        // Save rendered image
        lm::save("film1", opt["out"]);
    }
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }

    return 0;
}
