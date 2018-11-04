/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>

int main(int argc, char** argv) {
    // Initialize the framework
    lm::init({{"numThreads", -1}});

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

    // ------------------------------------------------------------------------

    // Define assets

    // Film for the rendered image
    const int w = opt["w"];
    const int h = opt["h"];
    lm::asset("film1", "film::bitmap", {
        {"w", w},
        {"h", h}
    });

    // Pinhole camera
    lm::asset("camera1", "camera::pinhole", {
        {"position", opt["eye"]},
        {"center", opt["lookat"]},
        {"up", {0,1,0}},
        {"vfov", opt["vfov"]},
        {"aspect", lm::Float(w) / h}
    });

    // OBJ model
    lm::asset("obj1", "model::wavefrontobj", {{"path", opt["obj"]}});
    
    // ------------------------------------------------------------------------

    // Define scene primitives

    // Camera
    lm::primitive(lm::Mat4(1), {{"camera", "camera1"}});

    // Create primitives from model asset
    lm::primitives(lm::Mat4(1), "obj1");

    // ------------------------------------------------------------------------

    // Render an image
    lm::render("renderer::raycast", "accel::sahbvh", {
        {"output", "film1"},
        {"color", {0,0,0}}
    });

    // Save rendered image
    lm::save("film1", opt["out"]);

    return 0;
}