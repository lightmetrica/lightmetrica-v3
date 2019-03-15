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
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
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
    bool shade_ = true;
    std::optional<GLuint> texture_;

public:
    GLMaterial(glm::vec3 color, bool wireframe, bool shade)
        : color_(color)
        , wireframe_(wireframe)
        , shade_(shade)
    {}

    GLMaterial(lm::Material* material) {
        if (material->key() != "material::wavefrontobj") {
            color_ = glm::vec3(0);
            return;
        }
        
        // For material::wavefrontobj, we try to use underlying texture
        auto* diffuse = dynamic_cast<lm::Material*>(material->underlying("diffuse"));
        if (!diffuse) {
            color_ = glm::vec3(0);
            return;
        }
        auto* tex = dynamic_cast<lm::Texture*>(diffuse->underlying("mapKd"));
        if (!tex) {
            color_ = *diffuse->reflectance({}, {});
            return;
        }

        // Create OpenGL texture
        const auto [w, h, c, data] = tex->buffer();

#if 0
        // Convert the texture to float type
        std::vector<float> data_(w*h * 3);
        for (int i = 0; i < w*h*3; i++) {
            data_[i] = float(data[i]);
        }
#endif

        texture_ = 0;
        glGenTextures(1, &*texture_);
        glBindTexture(GL_TEXTURE_2D, *texture_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        if (c == 3) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_FLOAT, data);
        }
        else if (c == 4) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_FLOAT, data);
        }
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
        glProgramUniform1i(name, glGetUniformLocation(name, "Shade"), shade_ ? 1 : 0);
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
        Triangles = 1<<0,
        LineStrip = 1<<1,
        Lines     = 1<<2,
        Points    = 1<<3,
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

    GLMesh(int type, const std::vector<lm::Vec3>& vs) : type_(type) {
        std::vector<GLuint> is(vs.size());
        std::iota(is.begin(), is.end(), 0);
        createGLBuffers(vs, {}, {}, is);
    }

    GLMesh(lm::Mesh* mesh) : type_(MeshType::Triangles) {
        // Create OpenGL buffer objects
        std::vector<lm::Vec3> vs;
        std::vector<lm::Vec3> ns;
        std::vector<lm::Vec2> ts;
        std::vector<GLuint> is;
        GLuint count = 0;
        mesh->foreachTriangle([&](int, const lm::Mesh::Tri& tri) {
            vs.insert(vs.end(), { tri.p1.p, tri.p2.p, tri.p3.p });
            ns.insert(ns.end(), { tri.p1.n, tri.p2.n, tri.p3.n });
            ts.insert(ts.end(), { tri.p1.t, tri.p2.t, tri.p3.t });
            is.insert(is.end(), { count, count+1, count+2 });
            count+=3;
        });
        createGLBuffers(vs, ns, ts, is);
    }

    ~GLMesh() {
        glDeleteVertexArrays(1, &vertexArray_);
        glDeleteBuffers(1, &bufferP_);
        glDeleteBuffers(1, &bufferN_);
        glDeleteBuffers(1, &bufferT_);
        glDeleteBuffers(1, &bufferI_);
    }

private:
    void createGLBuffers(
        const std::vector<lm::Vec3>& vs,
        const std::vector<lm::Vec3>& ns,
        const std::vector<lm::Vec2>& ts,
        const std::vector<GLuint>& is)
    {
        glGenBuffers(1, &bufferP_);
        glBindBuffer(GL_ARRAY_BUFFER, bufferP_);
        glBufferData(GL_ARRAY_BUFFER, vs.size() * sizeof(lm::Vec3), vs.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        CHECK_GL_ERROR();

        if (!ns.empty()) {
            glGenBuffers(1, &bufferN_);
            glBindBuffer(GL_ARRAY_BUFFER, bufferN_);
            glBufferData(GL_ARRAY_BUFFER, ns.size() * sizeof(lm::Vec3), ns.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            CHECK_GL_ERROR();
        }

        if (!ts.empty()) {
            glGenBuffers(1, &bufferT_);
            glBindBuffer(GL_ARRAY_BUFFER, bufferT_);
            glBufferData(GL_ARRAY_BUFFER, ts.size() * sizeof(lm::Vec2), ts.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            CHECK_GL_ERROR();
        }

        count_ = int(is.size());
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
        if (!ns.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, bufferN_);
            glVertexAttribPointer(1, 3, LM_DOUBLE_PRECISION ? GL_DOUBLE : GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(1);
        }
        if (!ts.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, bufferT_);
            glVertexAttribPointer(2, 2, LM_DOUBLE_PRECISION ? GL_DOUBLE : GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(2);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        CHECK_GL_ERROR();
    }

public:
    // Dispatch rendering
    void render() const {
        glBindVertexArray(vertexArray_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferI_);
        if ((type_ & MeshType::Triangles) > 0) {
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
    lm::Mat4 transform;
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
    // Reset the internal state
    void reset() {
        meshes_.clear();
        materials_.clear();
        materialMap_.clear();
        primitives_.clear();
    }

    // Add mesh and material pair
    void add(const lm::Mat4& transform, lm::Mesh* mesh, lm::Material* material) {
        LM_INFO("Creating GL primitive [#{}]", primitives_.size());

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

    void add(int type, lm::Vec3 color, const std::vector<lm::Vec3>& vs) {
        LM_INFO("Creating GL primitive [#{}]", primitives_.size());
        meshes_.emplace_back(new GLMesh(type, vs));
        materials_.emplace_back(new GLMaterial(color, true, false));
        primitives_.push_back({lm::Mat4(1_f), meshes_.back().get(), materials_.back().get()});
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
    lm::Float aspect_;
    lm::Float fov_;
    lm::Vec3 eye_;
    lm::Vec3 up_;
    lm::Vec3 forward_;
    lm::Float pitch_;
    lm::Float yaw_;

public:
    void reset(lm::Vec3 eye, lm::Vec3 center, lm::Vec3 up, lm::Float fov) {
        eye_ = eye;
        up_ = up;
        forward_ = glm::normalize(center - eye);
        fov_ = fov;
        pitch_ = glm::degrees(std::asin(forward_.y));
        yaw_ = glm::degrees(std::atan2(forward_.z, forward_.x));
    }

    lm::Vec3 eye() { return eye_; }
    lm::Vec3 center() { return eye_ + forward_; }
    lm::Float fov() { return fov_; }

    lm::Mat4 viewMatrix() const {
        return glm::lookAt(eye_, eye_ + forward_, up_);
    }
    
    lm::Mat4 projectionMatrix() const {
        return glm::perspective(glm::radians(fov_), aspect_, 0.01_f, 10000_f);
    }

    // True if the camera is updated
    void update(GLFWwindow* window) {
        // Update aspect ratio
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        aspect_ = lm::Float(display_w) / display_h;

        // Update forward vector
        {
            static auto prevMousePos = ImGui::GetMousePos();
            const auto mousePos = ImGui::GetMousePos();
            const bool rotating = ImGui::IsMouseDown(GLFW_MOUSE_BUTTON_RIGHT);
            if (rotating) {
                int w, h;
                glfwGetFramebufferSize(window, &w, &h);
                const float sensitivity = 0.1f;
                const float dx = float(prevMousePos.x - mousePos.x) * sensitivity;
                const float dy = float(prevMousePos.y - mousePos.y) * sensitivity;
                yaw_ += dx;
                pitch_ = glm::clamp(pitch_ - dy, -89_f, 89_f);
            }
            prevMousePos = mousePos;
            forward_ = glm::vec3(
                cos(glm::radians(pitch_)) * cos(glm::radians(yaw_)),
                sin(glm::radians(pitch_)),
                cos(glm::radians(pitch_)) * sin(glm::radians(yaw_)));
        }

        // Update camera position
        {
            const auto w = -forward_;
            const auto u = glm::normalize(glm::cross(up_, w));
            const auto v = glm::cross(w, u);
            const auto factor = ImGui::GetIO().KeyShift ? 10.0_f : 1_f;
            const auto speed = ImGui::GetIO().DeltaTime * factor;
            if (ImGui::IsKeyDown('W')) { eye_ += forward_ * speed; }
            if (ImGui::IsKeyDown('S')) { eye_ -= forward_ * speed; }
            if (ImGui::IsKeyDown('A')) { eye_ -= u * speed; }
            if (ImGui::IsKeyDown('D')) { eye_ += u * speed; }
        }
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
    bool setup() {
        // Load shaders
        const std::string vscode = R"x(
            #version 330 core
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
            #version 330 core
            in vec3 normal;
            in vec2 uv;
            out vec4 fragColor;
            uniform sampler2D tex;
            uniform vec3 Color;
            uniform int UseTexture;
            uniform int Shade;
            void main() {
                fragColor.rgb = Color;
                if (UseTexture == 0) {
                    fragColor.rgb = Color;
                }
                else {
                    fragColor.rgb = texture(tex, uv).rgb;
                }
                if (Shade == 1) {
                    fragColor.rgb *= .2+.8*max(0, dot(normal, vec3(0,0,1)));
                }
                fragColor.a = 1;
            }
        )x";

        const auto createProgram = [](GLenum shaderType, const std::string& code) -> std::optional<GLuint> {
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
                return {};
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
                return {};
            }
            return program;
        };

        if (auto p = createProgram(GL_VERTEX_SHADER, vscode); p) {
            progV_ = *p;
        }
        else {
            return false;
        }
        if (auto p = createProgram(GL_FRAGMENT_SHADER, fscode); p) {
            progF_ = *p;
        }
        else {
            return false;
        }
        glGenProgramPipelines(1, &pipeline_);
        glUseProgramStages(pipeline_, GL_VERTEX_SHADER_BIT, progV_);
        glUseProgramStages(pipeline_, GL_FRAGMENT_SHADER_BIT, progF_);

        CHECK_GL_ERROR();
        return true;
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

// Base class for interactive examples
class InteractiveApp {
public:
    GLFWwindow* window;
    GLScene glscene;
    GLRenderer glrenderer;
    GLDisplayCamera glcamera;

public:
    bool setup(const std::string& title, const lm::Json& opt) {
        // Init GLFW
        if (!glfwInit()) {
            return false;
        }

        // Error callback
        glfwSetErrorCallback([](int error, const char* desc) {
            LM_ERROR("[GLFW error #{}] {}", error, desc);
        });

        // Craete GLFW window
        window = [&]() -> GLFWwindow* {
            // GLFW window
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            #ifdef LM_DEBUG_MODE
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
            #endif
            GLFWwindow* window = glfwCreateWindow(opt["w"], opt["h"], title.c_str(), nullptr, nullptr);
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
            glfwTerminate();
            return false;
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

        // GL renderer
        if (!glrenderer.setup()) {
            return false;
        }

        // GL camera
        glcamera.reset(opt["eye"], opt["lookat"], lm::Vec3(0, 1, 0), opt["vfov"]);

        return true;
    }

    void run(const std::function<void(int, int)>& updateFunc) {
        while (!glfwWindowShouldClose(window)) {
            // Setup new frame
            glfwPollEvents();
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // ----------------------------------------------------------------

            // Update camera
            glcamera.update(window);

            // Windows position and size
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(350, 200), ImGuiCond_Once);

            // ----------------------------------------------------------------

            // General information
            ImGui::Begin("Information / Control");
            ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            ImGui::Text("Framebuffer size: (%d, %d)", display_w, display_h);
            ImGui::End();

            // ----------------------------------------------------------------

            updateFunc(display_w, display_h);

            // ----------------------------------------------------------------

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
        };

        // --------------------------------------------------------------------

        // Shutdown the framework
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};