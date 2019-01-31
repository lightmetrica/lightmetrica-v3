/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
using namespace lm::literals;

// ----------------------------------------------------------------------------

class Camera_Display final : public lm::Camera {
private:
    lm::Float aspect_;
    lm::Float fov_;
    lm::Vec3 eye_;
    lm::Vec3 up_;
    lm::Vec3 forward_;

public:
    virtual bool construct(const lm::Json& prop) override {
        eye_ = prop["position"];
        const lm::Vec3 center = prop["center"];
        up_ = prop["up"];
        fov_ = prop["vfov"];
        forward_ = glm::normalize(center - eye_);
        return true;
    }

    virtual lm::Mat4 viewMatrix() const override {
        return glm::lookAt(eye_, eye_ + forward_, up_);
    }
    
    virtual lm::Mat4 projectionMatrix() const override {
        return glm::perspective(glm::radians(fov_), aspect_, 0.01_f, 10000_f);
    }

public:
    void update(GLFWwindow* window) {
        // Update aspect ratio
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        aspect_ = lm::Float(display_w) / display_h;

        // Update forward vector
        forward_ = [&]() -> lm::Vec3 {
            static auto pitch = glm::degrees(std::asin(forward_.y));
            static auto yaw = glm::degrees(std::atan2(forward_.z, forward_.x));
            static auto prevMousePos = ImGui::GetMousePos();
            const auto mousePos = ImGui::GetMousePos();
            const bool rotating = ImGui::IsMouseDown(GLFW_MOUSE_BUTTON_RIGHT);
            if (rotating) {
                int w, h;
                glfwGetFramebufferSize(window, &w, &h);
                const float sensitivity = 0.1f;
                const float dx = float(prevMousePos.x - mousePos.x) * sensitivity;
                const float dy = float(prevMousePos.y - mousePos.y) * sensitivity;
                yaw += dx;
                pitch = glm::clamp(pitch - dy, -89_f, 89_f);
            }
            prevMousePos = mousePos;
            return glm::vec3(
                cos(glm::radians(pitch)) * cos(glm::radians(yaw)),
                sin(glm::radians(pitch)),
                cos(glm::radians(pitch)) * sin(glm::radians(yaw)));
        }();

        // Update camera position
        eye_ = [&]() -> lm::Vec3 {
            static auto p = eye_;
            const auto w = -forward_;
            const auto u = glm::normalize(glm::cross(up_, w));
            const auto v = glm::cross(w, u);
            const auto factor = ImGui::GetIO().KeyShift ? 10.0_f : 1_f;
            const auto speed = ImGui::GetIO().DeltaTime * factor;
            if (ImGui::IsKeyDown('W')) { p += forward_ * speed; }
            if (ImGui::IsKeyDown('S')) { p -= forward_ * speed; }
            if (ImGui::IsKeyDown('A')) { p -= u * speed; }
            if (ImGui::IsKeyDown('D')) { p += u * speed; }
            return p;
        }();
    }
};

LM_COMP_REG_IMPL(Camera_Display, "camera::display");

// ----------------------------------------------------------------------------

/*
    This example illustrates interactive visualization support.
*/
int main(int argc, char** argv) {
    try {
        // Initialize the framework
        lm::init();

        // We need `lmgl` plugin to use interactive features
        lm::comp::detail::ScopedLoadPlugin pluginGuard("lmgl");

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

        // Init GLFW
        if (!glfwInit()) {
            lm::shutdown();
            return EXIT_FAILURE;
        }

        // Craete GLFW window
        auto* window = [&]() -> GLFWwindow* {
            // GLFW window
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            #ifdef LM_DEBUG_MODE
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
            #endif
            GLFWwindow* window = glfwCreateWindow(opt["w"], opt["h"], "interactive", nullptr, nullptr);
            if (!window) { return nullptr; }
            glfwMakeContextCurrent(window);
            glfwSwapInterval(0);
            gl3wInit();
            // ImGui context
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGui_ImplGlfw_InitForOpenGL(window, true);
            ImGui_ImplOpenGL3_Init();
            ImGui::StyleColorsDark();
            return window;
        }();
        if (!window) {
            lm::shutdown();
            glfwTerminate();
            return EXIT_FAILURE;
        }

        #if LM_DEBUG_MODE
        // Debug output of OpenGL
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback([](GLenum source, GLenum type, GLuint, GLenum severity, GLsizei, const GLchar* message, void*) {
            const auto str = fmt::format("GL callback: {} [source={}, type={}, severity={}]", message, source, type, severity);
            if (type == GL_DEBUG_TYPE_ERROR) {
                LM_ERROR(str);
            }
            else {
                LM_INFO(str);
            }
        }, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, true);
        #endif

        // --------------------------------------------------------------------

        // OBJ model
        lm::asset("obj1", "model::wavefrontobj", {
            {"path", opt["obj"]}
        });

        // Camera for interactive display
        lm::asset("camera_display", "camera::display", {
            {"film", "film1"},
            {"position", opt["eye"]},
            {"center", opt["lookat"]},
            {"up", {0,1,0}},
            {"vfov", opt["vfov"]}
        });

        // Camera
        lm::primitive(lm::Mat4(1), {
            {"camera", "camera_display"}
        });

        // Create OpenGL-ready assets and register primitives
        auto* model = lm::getAsset<lm::Model>("obj1");
        model->createPrimitives([&](lm::Component* mesh, lm::Component* material, lm::Component*) {
            // Name of OpenGL assets
            const auto glmesh = "gl_" + mesh->name();
            const auto glmat  = "gl_" + material->name();

            // OpenGL mesh
            lm::asset(glmesh, "mesh::visgl", {
                {"mesh", mesh->globalLoc()}
            });

            // OpenGL material
            if (auto* mat = lm::getAsset<lm::Material>(glmat); !mat) {
                lm::asset(glmat, "material::visgl", {
                    {"material", material->globalLoc()}
                });
            }

            // Primitive
            lm::primitive(lm::Mat4(1), {
                {"mesh", glmesh},
                {"material", glmat}
            });
        });
        
        // --------------------------------------------------------------------

        // Render an image
        lm::renderer("renderer::visgl", {
            {"camera", "camera_display"}
        });

        // Main loop
        while (!glfwWindowShouldClose(window)) {
            // Setup new frame
            glfwPollEvents();
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Update camera
            auto* camera = lm::getAsset<Camera_Display>("camera_display");
            camera->update(window);

            // Windows position and size
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(350, 350), ImGuiCond_Once);

            // General information
            ImGui::Begin("Information / Control");
            ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            ImGui::Text("Framebuffer size: (%d, %d)", display_w, display_h);
            ImGui::Separator();
            ImGui::End();

            // Rendering
            ImGui::Render();
            glViewport(0, 0, display_w, display_h);
            glClearDepthf(1.f);
            glClear(GL_DEPTH_BUFFER_BIT);
            glClearColor(.45f, .55f, .6f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT);
            lm::render(false);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window);
        }

        // --------------------------------------------------------------------

        // Shutdown the framework
        lm::shutdown();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }

    return EXIT_SUCCESS;
}
