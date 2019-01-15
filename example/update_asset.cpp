/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>
#include <filesystem>

/*
    This example illustrats how to update the asset after initialization. 
    Command line arguments are same as `raycast.cpp`.
*/
int main(int argc, char** argv) {
    try {
        // Initialize the framework
        lm::init("user::default", {
            #if LM_DEBUG_MODE
            {"numThreads", -1}
            #else
            {"numThreads", -1}
            #endif
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
        // Load internal state
        lm::deserialize("lm.serialized");
        #else
        // Film for the rendered image
        lm::asset("film1", "film::bitmap", {
            {"w", opt["w"]},
            {"h", opt["h"]}
        });

        // Pinhole camera
        lm::asset("camera1", "camera::pinhole", {
            {"film", "film1"},
            {"position", opt["eye"]},
            {"center", opt["lookat"]},
            {"up", {0,1,0}},
            {"vfov", opt["vfov"]}
        });

		// Base material
		lm::asset("obj_base_mat", "material::diffuse", {
            {"Kd", {.8,.2,.2}}
        });

        // OBJ model
        lm::asset("obj1", "model::wavefrontobj", {
            {"path", opt["obj"]},
            {"base_material", "obj_base_mat"}
        });

        // Camera
        lm::primitive(lm::Mat4(1), { {"camera", "camera1"} });

        // Create primitives from model asset
        lm::primitives(lm::Mat4(1), "obj1");

        // Build acceleration structure
        lm::build("accel::sahbvh");

        // Save internal state for the debug mode
        lm::serialize("lm.serialized");
        #endif

        // --------------------------------------------------------------------

        const auto appended = [&](const std::string& mod) -> std::string {
            std::filesystem::path p(opt["out"].get<std::string>());
            const auto p2 = (p.parent_path() / (p.stem().string() + mod)).replace_extension(p.extension());
            return p2.string();
        };

        // --------------------------------------------------------------------

        // Render and save
        lm::render("renderer::raycast", {
            {"output", "film1"}
        });
        lm::save("film1", appended("_1"));

        // Replace `obj_base_mat` with different color
        // Note that this is not trivial, because `model::wavefrontobj`
        // already holds a reference to the original material.
        lm::asset("obj_base_mat", "material::diffuse", {
            {"Kd", {.2,.8,.2}}
        });

        // Render again
        lm::render("renderer::raycast", {
            {"output", "film1"}
        });
        lm::save("film1", appended("_2"));

    }
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }

    return 0;
}