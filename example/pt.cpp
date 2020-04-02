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
            {"num_threads", 1}
            #else
            {"num_threads", -1}
            #endif
        });
        lm::info();

        // Parse command line arguments
        const auto opt = lm::json::parse_positional_args<13>(argc, argv, R"({{
            "obj": "{}",
            "out": "{}",
            "spp": {},
            "max_verts": {},
            "w": {},
            "h": {},
            "eye": [{},{},{}],
            "lookat": [{},{},{}],
            "vfov": {}
        }})");

        // ----------------------------------------------------------------------------------------

        // Define assets

        // Film for the rendered image
        const auto* film = lm::load<lm::Film>("film1", "film::bitmap", {
            {"w", opt["w"]},
            {"h", opt["h"]}
        });

        // Pinhole camera
        const lm::Float w = opt["w"];
        const lm::Float h = opt["h"];
        const auto* camera = lm::load<lm::Camera>("camera1", "camera::pinhole", {
            {"film", film->loc()},
            {"position", opt["eye"]},
            {"center", opt["lookat"]},
            {"up", {0,1,0}},
            {"vfov", opt["vfov"]},
            {"aspect", w/h}
        });

        // OBJ model
        const auto* model = lm::load<lm::Model>("obj1", "model::wavefrontobj", {
            {"path", opt["obj"]}
        });

        // Define scene primitives
        const auto* accel = lm::load<lm::Accel>("accel", "accel::sahbvh", {});
        auto* scene = lm::load<lm::Scene>("scene", "scene::default", {
            {"accel", accel->loc()}
        });

        // Camera
        scene->add_primitive({
            {"camera", camera->loc()}
        });

        // Create primitives from model asset
        scene->add_primitive({
            {"model", model->loc()}
        });

        // Build acceleration structure
        scene->build();

        // ----------------------------------------------------------------------------------------

        // Render an image
        const auto* renderer = lm::load<lm::Renderer>("renderer", "renderer::pt", {
            {"output", film->loc()},
            {"scene", scene->loc()},
            {"scheduler", "sample"},
            {"spp", opt["spp"]},
            {"max_verts", opt["max_verts"]}
        });
        renderer->render();

        // Save rendered image
        film->save(opt["out"]);

        // Shutdown the framework
        lm::shutdown();
    }
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }

    return 0;
}