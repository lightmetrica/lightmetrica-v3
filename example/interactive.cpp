/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>
#include <GLFW/glfw3.h>

/*
    This example illustrates interactive visualization support.
*/
int main(int argc, char** argv) {
    // Initialize GLFW

    // Init GLFW
    if (!glfwInit()) { return EXIT_FAILURE; }

    // Craete GLFW window
    auto* window = [&]() -> GLFWwindow*
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        GLFWwindow* window = glfwCreateWindow(1980, 1080, "vis", NULL, NULL);
        if (!window) { return nullptr; }
        glfwMakeContextCurrent(window);
        gl3wInit();
        ImGui_ImplGlfwGL3_Init(window, true);
        return window;
    }();
    if (!window)
    {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    // ----------------------------------------------------------------------------

    try {
        // Initialize the framework
        lm::init();

        // We need `lmgl` plugin to use interactive features
        lm::comp::detail::ScopedLoadPlugin pluginGuard("lmgl");

        // Parse command line arguments
        const auto opt = lm::json::parsePositionalArgs<13>(argc, argv, R"({{
            "obj": "{}",
            "out": "{}",
            "spp": {},
            "w": {},
            "h": {},
            "eye": [{},{},{}],
            "lookat": [{},{},{}],
            "vfov": {}
        }})");

        // --------------------------------------------------------------------

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
        // Replace all materials to diffuse and use our checker texture
        lm::asset("obj1", "model::wavefrontobj", {
            {"path", opt["obj"]}
        });

        // --------------------------------------------------------------------

        // Define scene primitives

        // Camera
        lm::primitive(lm::Mat4(1), { {"camera", "camera1"} });

        // Create OpenGL-ready assets and register primitives
        auto* model = lm::getAsset<lm::Model>("obj1");
        model->createPrimitives([&](lm::Component* mesh, lm::Component* material, lm::Component*) {
            // Extract the name of the asset from locator
            const auto extractName = [](lm::Component* comp) -> std::string {
                const auto loc = comp->loc();
                const auto i = loc.find_last_of('.');
                assert(i != std::string::npos);
                return loc.substr(i);
            };
            const auto glmesh = extractName(mesh);
            const auto glmaterial = extractName(material);
            lm::asset(glmesh, "mesh::visgl", {
                {"mesh", mesh->loc()}
            });
            lm::asset(glmaterial, "material::visgl", {
                {"material", mesh->loc()}
            });
            
            // Add primitive
            lm::primitive(lm::Mat4(1), {
                {"mesh", glmesh},
                {"material", glmaterial}
            });
        });
        
        // --------------------------------------------------------------------

        // Render an image
        lm::build("accel::sahbvh");
        lm::render("renderer::ao2", {
            {"output", "film1"},
            {"spp", opt["spp"]}
        });

        // Save rendered image
        lm::save("film1", opt["out"]);

        // Shutdown the framework
        lm::shutdown();
    }
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }

    return 0;
}
