/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "lmgl.h"
using namespace lm;
using namespace lm::literals;

#define THROW_RUNTIME_ERROR() \
    throw std::runtime_error("Consult log outputs for detailed error messages")

// ----------------------------------------------------------------------------

// OpenGL material
class GLMaterial {
private:
    Material* material_;

public:
    GLMaterial(Material* material) : material_(material) {}

public:
    // Enable material parameters
    void apply(const std::function<void()>& func) const {
        func();
    }
};

// ----------------------------------------------------------------------------

// OpenGL mesh
namespace MeshType {
    enum {
        Triangles  = 1<<0,
        LineStrip  = 1<<1,
        Lines      = 1<<2,
        Points     = 1<<3,
    };
}
class GLMesh {
private:
    int type_;
    int count_;
    GLResource bufferP_;
    GLResource bufferN_;
    GLResource bufferT_;
    GLResource vertexArray_;

public:
    GLMesh(Mesh* mesh) {
        // Mesh type
        //type_ = prop["type"];
        type_ = MeshType::Triangles;
        
        // Create OpenGL buffer objects
        std::vector<Float> vs;
        mesh->foreachTriangle([&](int, Vec3 p1, Vec3 p2, Vec3 p3) {
            vs.insert(vs.end(), {
                p1.x, p1.y, p1.z,
                p2.x, p2.y, p2.z,
                p3.x, p3.y, p3.z
            });
        });
        count_ = int(vs.size()) / 3;
        bufferP_.create(GLResourceType::ArrayBuffer);
        bufferP_.allocate(vs.size() * sizeof(double), &vs[0], GL_DYNAMIC_DRAW);
        vertexArray_.create(GLResourceType::VertexArray);
        vertexArray_.addVertexAttribute(bufferP_, 0, 3, LM_DOUBLE_PRECISION ? GL_DOUBLE : GL_FLOAT, GL_FALSE, 0, nullptr);
    }

    ~GLMesh() {
        vertexArray_.destory();
        bufferP_.destory();
        bufferN_.destory();
        bufferT_.destory();
    }

public:
    // Dispatch rendering
    void render() const {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        //if (Params.WireFrame) {
        //    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        //}
        //else {
        //    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        //}
        if ((type_ & MeshType::Triangles) > 0) {
            vertexArray_.draw(GL_TRIANGLES, 0, count_);
        }
        if ((type_ & MeshType::LineStrip) > 0) {
            vertexArray_.draw(GL_LINE_STRIP, 0, count_);
        }
        if ((type_ & MeshType::Lines) > 0) {
            vertexArray_.draw(GL_LINES, 0, count_);
        }
        if ((type_ & MeshType::Points) > 0) {
            vertexArray_.draw(GL_POINTS, 0, count_);
        }
    }
};

// ----------------------------------------------------------------------------

// OpenGL scene
struct GLPrimitive {
    Mat4 transform;
    GLMesh* mesh;
    GLMaterial* material;
};
class GLScene {
private:
    std::vector<std::unique_ptr<GLMesh>> meshes_;
    std::vector<std::unique_ptr<GLMaterial>> materials_;
    std::unordered_map<std::string, int> materialMap_;
    std::vector<GLPrimitive> primitives_;

public:
    // Add mesh and material pair
    void add(const Mat4& transform, Mesh* mesh, Material* material) {
        // Mesh
        auto* glmesh = [&]() {
            meshes_.emplace_back(new GLMesh(mesh));
            return meshes_.back().get();
        }();

        // Material
        auto* glmaterial = [&]() {
            if (auto it = materialMap_.find(material->name()); it != materialMap_.end()) {
                return materials_.at(it->second).get();
            }
            materialMap_[material->name()] = int(materials_.size());
            materials_.emplace_back(new GLMaterial(material));
            return materials_.back().get();
        }();
        
        // Primitive
        primitives_.push_back({transform, glmesh, glmaterial});
    }

    // Iterate primitives
    using ProcessPrimitiveFunc = std::function<void(const GLPrimitive& primitive)>;
    void foreachPrimitive(const ProcessPrimitiveFunc& processPrimitive) const {
        for (const auto& primitive : primitives_) {
            processPrimitive(primitive);
        }
    }
};

// ----------------------------------------------------------------------------

class GLDisplayCamera {
private:
    Float aspect_;
    Float fov_;
    Vec3 eye_;
    Vec3 up_;
    Vec3 forward_;

public:
    GLDisplayCamera(Vec3 eye, Vec3 center, Vec3 up, Float fov) 
        : eye_(eye)
        , up_(up)
        , forward_(glm::normalize(center - eye))
        , fov_(fov)
    {}

    Mat4 viewMatrix() const {
        return glm::lookAt(eye_, eye_ + forward_, up_);
    }
    
    Mat4 projectionMatrix() const {
        return glm::perspective(glm::radians(fov_), aspect_, 0.01_f, 10000_f);
    }

    // True if the camera is updated
    bool update(GLFWwindow* window) {
        bool updated = false;

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
                updated = true;
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
            if (ImGui::IsKeyDown('W')) { updated = true; p += forward_ * speed; }
            if (ImGui::IsKeyDown('S')) { updated = true; p -= forward_ * speed; }
            if (ImGui::IsKeyDown('A')) { updated = true; p -= u * speed; }
            if (ImGui::IsKeyDown('D')) { updated = true; p += u * speed; }
            return p;
        }();

        return updated;
    }
};

// ----------------------------------------------------------------------------

// Interactive visualizer using OpenGL
class GLRenderer {
private:
    GLResource pipeline_;
    mutable GLResource programV_;
    mutable GLResource programF_;

public:
    GLRenderer() {
        // Load shaders
        const std::string RenderVs = R"x(
            #version 400 core
            #define POSITION 0
            #define NORMAL   1
            #define TEXCOORD 2
            layout (location = POSITION) in vec3 position;
            layout (location = NORMAL) in vec3 normal;
            layout (location = TEXCOORD) in vec2 texcoord;
            uniform mat4 ModelMatrix;
            uniform mat4 ViewMatrix;
            uniform mat4 ProjectionMatrix;
            out block {
                vec3 normal;
                vec2 texcoord;
            } Out;
            void main() {
                mat4 mvMatrix = ViewMatrix * ModelMatrix;
                mat4 mvpMatrix = ProjectionMatrix * mvMatrix;
                Out.normal = normal;//mat3(mvMatrix) * normal;
                Out.texcoord = texcoord;
                gl_Position = mvpMatrix * vec4(position, 1);
            }
        )x";
        const std::string RenderFs = R"x(
            #version 400 core
            in block {
                vec3 normal;
                vec2 texcoord;
            } In;
            out vec4 fragColor;
            uniform vec3 Color;
            uniform float Alpha;
            uniform int UseConstantColor;
            void main() {
                fragColor.rgb = UseConstantColor > 0 ? Color : abs(In.normal);
                fragColor.a = Alpha;
            }
        )x";
        programV_.create(GLResourceType::Program);
        programF_.create(GLResourceType::Program);
        if (!programV_.compileString(GL_VERTEX_SHADER, RenderVs)) { THROW_RUNTIME_ERROR(); }
        if (!programF_.compileString(GL_FRAGMENT_SHADER, RenderFs)) { THROW_RUNTIME_ERROR(); }
        if (!programV_.link()) { THROW_RUNTIME_ERROR(); }
        if (!programF_.link()) { THROW_RUNTIME_ERROR(); }
        pipeline_.create(GLResourceType::Pipeline);
        pipeline_.addProgram(programV_);
        pipeline_.addProgram(programF_);
    }
    
    // This function is called one per frame
    void render(const GLScene& scene, const GLDisplayCamera& camera) const {
        // State
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Camera
        programV_.setUniform("ViewMatrix", glm::mat4(camera.viewMatrix()));
        programV_.setUniform("ProjectionMatrix", glm::mat4(camera.projectionMatrix()));

        // Render meshes
        pipeline_.bind();
        scene.foreachPrimitive([&](const GLPrimitive& p) {
            programV_.setUniform("ModelMatrix", glm::mat4(p.transform));
            programF_.setUniform("Color", glm::vec3(1));
            programF_.setUniform("Alpha", 1.f);
            programF_.setUniform("UseConstantColor", 1);
            p.material->apply([&]() {
                p.mesh->render();
            });
        });
        pipeline_.unbind();

        // Restore
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_BLEND);
    }
};

// ----------------------------------------------------------------------------

/*
    This example illustrates interactive visualization support.
*/
int main(int argc, char** argv) {
    try {
        // Initialize the framework
        lm::init();

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

        lm::primitives(lm::Mat4(1), "obj1");

        // --------------------------------------------------------------------

        // Setup renderer
        GLScene glscene;
        GLRenderer glrenderer;
        GLDisplayCamera glcamera(opt["eye"], opt["lookat"], Vec3(0, 1, 0), opt["vfov"]);

        // Create OpenGL-ready assets and register primitives
        const auto* scene = lm::comp::get<lm::Scene>("scene");
        scene->foreachPrimitive([&](const lm::Primitive& p) {
            if (!p.mesh || !p.material) {
                return;
            }
            glscene.add(p.transform.M, p.mesh, p.material);
        });
        
        // --------------------------------------------------------------------

        // Main loop
        while (!glfwWindowShouldClose(window)) {
            // Setup new frame
            glfwPollEvents();
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Update camera
            const bool cameraUpdated = glcamera.update(window);

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

#if 0
            // Dispatch rendering
            const bool rendering = lm::rendering("renderer::pt");
            if (ImGui::ButtonEx("Render", ImVec2(0, 0), rendering ? ImGuiButtonFlags_Disabled : 0)) {
                // Create film to store rendered image
                lm::asset("film_render", "film::bitmap", {
                    {"w", opt["w"]},
                    {"h", opt["h"]}
                });

                // Camera
                lm::asset("camera_render", "camera::pinhole", {
                    {"film", "camera_render"},
                    {"position", opt["eye"]},
                    {"center", opt["lookat"]},
                    {"up", {0,1,0}},
                    {"vfov", opt["vfov"]}
                });

                // Renderer
                lm::renderer("renderer::pt", {
                    {"output", "film1"},
                    {"spp", opt["spp"]},
                    {"maxLength", opt["len"]}
                });

                // Create a new thread and dispatch rendering
                std::thread([]() {
                    lm::render("renderer::pt", true);
                }).detach();
            }

            // If camera movement is detected, abort rendering and clear film
            if (rendering && cameraUpdated) {
                lm::abort("renderer::pt");
            }
#endif

            // Rendering
            ImGui::Render();
            glViewport(0, 0, display_w, display_h);
            glClearDepthf(1.f);
            glClear(GL_DEPTH_BUFFER_BIT);
            glClearColor(.45f, .55f, .6f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT);
            glrenderer.render(glscene, glcamera);
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
