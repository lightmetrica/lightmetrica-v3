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
using namespace lm;
using namespace lm::literals;

// ----------------------------------------------------------------------------

#define THROW_RUNTIME_ERROR() \
    throw std::runtime_error("Consult log outputs for detailed error messages")

static void checkGLError(const char* filename, const int line) {
    if (int err = glGetError(); err != GL_NO_ERROR) {
        LM_ERROR("OpenGL Error: {} {} {}", err, filename, line);
        THROW_RUNTIME_ERROR();
    }
}

#define CHECK_GL_ERROR() checkGLError(__FILE__, __LINE__)

// ----------------------------------------------------------------------------

// OpenGL material
class GLMaterial {
private:
    glm::vec3 color_;
    bool wireframe_ = false;
    std::optional<GLuint> texture_;

public:
    GLMaterial(Material* material) {
        if (material->key() != "material::wavefrontobj") {
            color_ = glm::vec3(0);
            return;
        }
        
        // For material::wavefrontobj, we try to use underlying texture
        auto* diffuse = dynamic_cast<Material*>(material->underlying("diffuse"));
        if (!diffuse) {
            color_ = glm::vec3(0);
            return;
        }
        auto* tex = dynamic_cast<Texture*>(diffuse->underlying("mapKd"));
        if (!tex) {
            color_ = *diffuse->reflectance({}, {});
            return;
        }

        // Create OpenGL texture
        const auto [w, h, data] = tex->buffer();

        // Convert the texture to float type
        std::vector<float> data_f(w*h * 3);
        for (int i = 0; i < w*h*3; i++) {
            data_f[i] = float(data[i]);
        }

        texture_ = 0;
        glGenTextures(1, &*texture_);
        glBindTexture(GL_TEXTURE_2D, *texture_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_FLOAT, data_f.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    ~GLMaterial() {
        if (texture_) {
            glDeleteTextures(1, &*texture_);
        }
    }

public:
    // Enable material parameters
    void apply(GLuint name, const std::function<void()>& process) const {
        if (wireframe_) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        glProgramUniform3fv(name, glGetUniformLocation(name, "Color"), 1, &color_.x);
        if (texture_) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, *texture_);
            glProgramUniform1i(name, glGetUniformLocation(name, "UseTexture"), 1);
        }
        else {
            glProgramUniform1i(name, glGetUniformLocation(name, "UseTexture"), 0);
        }
        process();
        if (texture_) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
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
    GLuint count_;
    GLuint bufferP_;
    GLuint bufferN_;
    GLuint bufferT_;
    GLuint bufferI_;
    GLuint vertexArray_;

public:
    GLMesh(Mesh* mesh) {
        // Mesh type
        //type_ = prop["type"];
        type_ = MeshType::Triangles;
        
        // Create OpenGL buffer objects
        std::vector<Vec3> vs;
        std::vector<Vec3> ns;
        std::vector<Vec2> ts;
        std::vector<GLuint> is;
        count_ = 0;
        mesh->foreachTriangle([&](int, Mesh::Point p1, Mesh::Point p2, Mesh::Point p3) {
            vs.insert(vs.end(), { p1.p, p2.p, p3.p });
            ns.insert(ns.end(), { p1.n, p2.n, p3.n });
            ts.insert(ts.end(), { p1.t, p2.t, p3.t });
            is.insert(is.end(), { count_, count_+1, count_+2 });
            count_+=3;
        });

        glGenBuffers(1, &bufferP_);
        glBindBuffer(GL_ARRAY_BUFFER, bufferP_);
        glBufferData(GL_ARRAY_BUFFER, vs.size() * sizeof(Vec3), vs.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        CHECK_GL_ERROR();

        glGenBuffers(1, &bufferN_);
        glBindBuffer(GL_ARRAY_BUFFER, bufferN_);
        glBufferData(GL_ARRAY_BUFFER, ns.size() * sizeof(Vec3), ns.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        CHECK_GL_ERROR();

        glGenBuffers(1, &bufferT_);
        glBindBuffer(GL_ARRAY_BUFFER, bufferT_);
        glBufferData(GL_ARRAY_BUFFER, ts.size() * sizeof(Vec2), ts.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        CHECK_GL_ERROR();

        glGenBuffers(1, &bufferI_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferI_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, is.size() * sizeof(GLuint), is.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        CHECK_GL_ERROR();

        glGenVertexArrays(1, &vertexArray_);
        glBindVertexArray(vertexArray_);
        glBindBuffer(GL_ARRAY_BUFFER, bufferP_);
        glVertexAttribPointer(0, 3, LM_DOUBLE_PRECISION ? GL_DOUBLE : GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, bufferN_);
        glVertexAttribPointer(1, 3, LM_DOUBLE_PRECISION ? GL_DOUBLE : GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, bufferT_);
        glVertexAttribPointer(2, 2, LM_DOUBLE_PRECISION ? GL_DOUBLE : GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        CHECK_GL_ERROR();
    }

    ~GLMesh() {
        glDeleteVertexArrays(1, &vertexArray_);
        glDeleteBuffers(1, &bufferP_);
        glDeleteBuffers(1, &bufferN_);
        glDeleteBuffers(1, &bufferT_);
        glDeleteBuffers(1, &bufferI_);
    }

public:
    // Dispatch rendering
    void render() const {
        glBindVertexArray(vertexArray_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferI_);
        if ((type_ & MeshType::Triangles) > 0) {
            //glDrawArrays(GL_TRIANGLES, 0, count_);
            glDrawElements(GL_TRIANGLES, count_, GL_UNSIGNED_INT, nullptr);
        }
        if ((type_ & MeshType::LineStrip) > 0) {
            glDrawElements(GL_LINE_STRIP, count_, GL_UNSIGNED_INT, nullptr);
        }
        if ((type_ & MeshType::Lines) > 0) {
            glDrawElements(GL_LINES, count_, GL_UNSIGNED_INT, nullptr);
        }
        if ((type_ & MeshType::Points) > 0) {
            glDrawElements(GL_POINTS, count_, GL_UNSIGNED_INT, nullptr);
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
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
    GLuint pipeline_;
    GLuint progV_;
    GLuint progF_;

public:
    GLRenderer() {
        // Load shaders
        const std::string vscode = R"x(
            #version 430 core
            layout (location = 0) in vec3 position_;
            layout (location = 1) in vec3 normal_;
            layout (location = 2) in vec2 uv_;
            out gl_PerVertex {
                vec4 gl_Position;
            };
            out vec3 normal;
            out vec2 uv;
            uniform mat4 ModelMatrix;
            uniform mat4 ViewMatrix;
            uniform mat4 ProjectionMatrix;
            void main() {
                mat4 mvMatrix = ViewMatrix * ModelMatrix;
                mat4 mvpMatrix = ProjectionMatrix * mvMatrix;
                mat3 normalMatrix = mat3(transpose(inverse(mvMatrix)));
                normal = normalMatrix * normal_;
                uv = uv_;
                gl_Position = mvpMatrix * vec4(position_, 1);
            }
        )x";
        const std::string fscode = R"x(
            #version 430 core
            in vec3 normal;
            in vec2 uv;
            out vec4 fragColor;
            layout (binding = 0) uniform sampler2D tex;
            uniform vec3 Color;
            uniform int UseTexture;
            void main() {
                fragColor.rgb = Color;
                if (UseTexture == 0)
                    fragColor.rgb = Color;
                else
                    fragColor.rgb = texture(tex, uv).rgb;
                //fragColor.rgb = abs(normal);
                fragColor.rgb *= .3+.7*max(0, dot(normal, vec3(0,0,1)));
                fragColor.a = 1;
            }
        )x";

        const auto createProgram = [](GLenum shaderType, const std::string& code) -> GLuint {
            GLuint program = glCreateProgram();
            GLuint shader = glCreateShader(shaderType);
            const auto* codeptr = code.c_str();
            glShaderSource(shader, 1, &codeptr, nullptr);
            glCompileShader(shader);
            GLint ret;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &ret);
            if (!ret) {
                int length;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
                std::vector<char> v(length);
                glGetShaderInfoLog(shader, length, nullptr, &v[0]);
                glDeleteShader(shader);
                LM_ERROR("{}", v.data());
                THROW_RUNTIME_ERROR();
            }
            glAttachShader(program, shader);
            glProgramParameteri(program, GL_PROGRAM_SEPARABLE, GL_TRUE);
            glDeleteShader(shader);
            glLinkProgram(program);
            glGetProgramiv(program, GL_LINK_STATUS, &ret);
            if (!ret) {
                GLint length;
                glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
                std::vector<char> v(length);
                glGetProgramInfoLog(program, length, nullptr, &v[0]);
                LM_ERROR("{}", v.data());
                THROW_RUNTIME_ERROR();
            }
            return program;
        };

        progV_ = createProgram(GL_VERTEX_SHADER, vscode);
        progF_ = createProgram(GL_FRAGMENT_SHADER, fscode);
        glGenProgramPipelines(1, &pipeline_);
        glUseProgramStages(pipeline_, GL_VERTEX_SHADER_BIT, progV_);
        glUseProgramStages(pipeline_, GL_FRAGMENT_SHADER_BIT, progF_);

        CHECK_GL_ERROR();
    }
    
    // This function is called one per frame
    void render(const GLScene& scene, const GLDisplayCamera& camera) const {
        // State
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Camera
        const auto viewM = glm::mat4(camera.viewMatrix());
        glProgramUniformMatrix4fv(progV_, glGetUniformLocation(progV_, "ViewMatrix"), 1, GL_FALSE, glm::value_ptr(viewM));
        const auto projM = glm::mat4(camera.projectionMatrix());
        glProgramUniformMatrix4fv(progV_, glGetUniformLocation(progV_, "ProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(projM));

        // Render meshes
        glBindProgramPipeline(pipeline_);
        scene.foreachPrimitive([&](const GLPrimitive& p) {
            glProgramUniformMatrix4fv(progV_, glGetUniformLocation(progV_, "ModelMatrix"), 1, GL_FALSE, glm::value_ptr(glm::mat4(p.transform)));
            p.material->apply(progF_, [&]() {
                p.mesh->render();
            });
        });
        glBindProgramPipeline(0);

        // Restore
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_BLEND);

        CHECK_GL_ERROR();
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
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
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
