/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>
#include <fstream>
#include <chrono>

/*
    This example illustrates how we can utilize serialization feature
    to reduce loading time in Debug mode.
*/
int main(int argc, char** argv) {
    try {
        // Initialize the framework
        lm::init("user::default", { {"numThreads", -1} });

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

        const auto setup = [&]() {
            // Define assets
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
            // OBJ model
            lm::asset("obj1", "model::wavefrontobj", { {"path", opt["obj"]} });

            // Define scene primitives
            // Camera
            lm::primitive(lm::Mat4(1), { {"camera", "camera1"} });
            // Create primitives from model asset
            lm::primitives(lm::Mat4(1), "obj1");

            // Build acceleration structure
            lm::build("accel::sahbvh");
        };

        const auto measure = [](const std::string& title, const std::function<void()>& func) {
            using namespace std::chrono;
            const auto start = high_resolution_clock::now();
            func();
            const auto end = high_resolution_clock::now();
            const auto elapsed = (double)duration_cast<milliseconds>(end - start).count() / 1000.0;
            LM_INFO("{}: {:.2f} s", title, elapsed);
        };

        const auto reset = []() {
            lm::shutdown();
            lm::init("user::default", { {"numThreads", -1} });
        };

        // --------------------------------------------------------------------

        #if !LM_DEBUG_MODE
        {
            // Load assets and build structures in Release mode,
            // then serialize the internal state to a file.
            measure("release", [&]() {
                setup();
                std::ofstream os("lm.serialized", std::ios::out | std::ios::binary);
                lm::serialize(os);
            });
        }
        #else
        {
            // Measure elapsed time for the two cases
            measure("debug-setup", [&]() {
                reset();
                setup();
            });
            measure("debug-deserialize", [&]() {
                reset();
                std::ifstream is("lm.serialized", std::ios::in | std::ios::binary);
                lm::deserialize(is);
            });
        }
        #endif

        // --------------------------------------------------------------------

        // Render an image
        lm::render("renderer::raycast", {
            {"output", "film1"},
            {"color", {0,0,0}}
        });

        // Save rendered image
        lm::save("film1", opt["out"]);
    }
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }

    return 0;
}