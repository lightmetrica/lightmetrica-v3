/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>

int main() {
    try {
        // Initialize the framework
        lm::init();

        // Define assets
        // Film for the rendered image
        lm::asset("film", "film::bitmap", {{"w", 1920}, {"h", 1080}});

        // Render an image
        lm::render("renderer::blank", {
            {"output", "film"},
            {"color", {1,1,1}}
        });

        // Save rendered image
        lm::save("film", "result.pfm");
    }
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }

    return 0;
}
