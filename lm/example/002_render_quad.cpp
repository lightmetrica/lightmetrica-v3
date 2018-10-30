/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>

int main(int argc, char** argv) {
    // Initialize the framework
    lm::init();

    // Define assets
    // Film for the rendered image
    lm::asset("film", "film::bitmap", {
        {"w", 1920},
        {"h", 1080}
    });
    // Load mesh with raw vertex data
    lm::asset("mesh1", "mesh::raw", {
        {"ps", {0,0,0,1,0,0,1,1,0,0,1,0}},
        {"ns", {0,0,1}},
        {"ts", {0,0,1,0,1,1,0,1}},
        {"fs", {
            {0,1,2,0,2,3},
            {0,0,0,0,0,0},
            {0,1,2,0,2,3}
        }}
    });

    // Define scene primitives
    lm::primitive("p1", lm::mat4(1), {
        {"mesh", "mesh1"}
    });

    // Render an image
    lm::render("renderer::raycast", "accel::naive", {
        {"output", "film"},
        {"color", lm::castToJson(lm::vec3(1))}
    });

    // Save rendered image
    lm::save("film", "result.pfm");

    // Finalize the framework
    lm::shutdown();

    return 0;
}
