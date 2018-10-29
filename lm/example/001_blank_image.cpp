/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>

int main(int argc, char** argv) {
    // Initialize the framework
    lm::init();

    // Asset
    lm::asset("film", "film::bitmap", {
        {"output", "result.pfm"},
        {"w", 1920},
        {"h", 1080}
    });
    
    // Render
    lm::render("renderer::blank", {
        { "output_film", "film" },
        { "color", { 1,0,0 } }
    });

    // Save rendered image
    lm::save("film");

    // Finalize the framework
    lm::shutdown();
}
