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

    // Render an image
    // We don't need acceleration structure so we keep second argument blank.
    lm::render("renderer::blank", "", {
        {"output", "film"},
        {"color", lm::castToJson(lm::Vec3(1))}
    });

    // Save rendered image
    lm::save("film", "result.pfm");

    // Finalize the framework
    lm::shutdown();

    return 0;
}
