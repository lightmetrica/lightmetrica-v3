/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>
#include "gl.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// ----------------------------------------------------------------------------

// OpenGL material
class Material_VisGL final : public Material {
public:


public:
    // Enable material parameters
    void apply(const std::function<void()>& func) const {

    }
};

LM_COMP_REG_IMPL(Material_VisGL, "material::visgl");

// ----------------------------------------------------------------------------

namespace MeshType {
    enum {
        Triangles  = 1<<0,
        LineStrip  = 1<<1,
        Lines      = 1<<2,
        Points     = 1<<3,
    };
}

// OpenGL mesh
class Mesh_VisGL final : public Mesh {
private:
    int type_;                  // Mesh type
    Mesh* mesh_;                // Reference to real mesh
    int count_;
    GLResource bufferP_;
    GLResource bufferN_;
    GLResource bufferT_;
    GLResource vertexArray_;

public:
    ~Mesh_VisGL() {
        vertexArray_.Destory();
        bufferP_.Destory();
        bufferN_.Destory();
        bufferT_.Destory();
    }

public:
    virtual bool construct(const Json& prop) override {
        // Mesh type
        type_ = prop["type"];
        
        // Referencing mesh
        mesh_ = getAsset<Mesh>(prop, "mesh");
        if (!mesh_) {
            return false;
        }

        // Create OpenGL buffer objects
        // TODO

        return true;
    }

public:
    // Dispatch rendering
    void render() const {
        
    }
};

LM_COMP_REG_IMPL(Mesh_VisGL, "mesh::visgl");

// ----------------------------------------------------------------------------

// Interactive visualizer using OpenGL
class Renderer_VisGL final : public Renderer {
private:
    GLResource pipeline_;
    GLResource programV_;
    GLResource programF_;

public:
    virtual bool construct(const Json& prop) override {
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
        programV_.Create(GLResourceType::Program);
        programF_.Create(GLResourceType::Program);
        if (!programV_.CompileString(GL_VERTEX_SHADER, RenderVs)) return false;
        if (!programF_.CompileString(GL_FRAGMENT_SHADER, RenderFs)) return false;
        if (!programV_.Link()) return false;
        if (!programF_.Link()) return false;
        pipeline_.Create(GLResourceType::Pipeline);
        pipeline_.AddProgram(programV_);
        pipeline_.AddProgram(programF_);

        return true;
    }
    
    // This function is called one per frame
    virtual void render(const Scene* scene) const override {
        // Clear buffer
        float depth(1.0f);
        glClearBufferfv(GL_DEPTH, 0, &depth);
        glClearBufferfv(GL_COLOR, 0, &glm::vec4(0.0f)[0]);

        // State
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Camera
        // TODO. project and view matrices

        // Render meshes
        scene->foreachPrimitive([](const Primitive& primitive) {
            const auto* mesh = dynamic_cast<Mesh_VisGL*>(primitive.mesh);
            const auto* material = dynamic_cast<Material_VisGL*>(primitive.material);
            if (!mesh || !material) {
                return;
            }
            material->apply([&]() {
                mesh->render();
            });
        });

        // Restore
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_BLEND);
    }
};

LM_COMP_REG_IMPL(Renderer_VisGL, "renderer::visgl");

LM_NAMESPACE_END(LM_NAMESPACE)
