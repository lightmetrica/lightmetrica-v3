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
private:
    Material* material_;

public:
    virtual bool construct(const Json& prop) override {
        material_ = getAsset<Material>(prop, "material");
        if (!material_) {
            return false;
        }

        return true;
    }

public:
    // Enable material parameters
    void apply(const std::function<void()>& func) const {
        func();
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
    bool init_ = false;

public:
    ~Mesh_VisGL() {
        if (init_) {
            vertexArray_.destory();
            bufferP_.destory();
            bufferN_.destory();
            bufferT_.destory();
        }
    }

public:
    virtual bool construct(const Json& prop) override {
        // Mesh type
        //type_ = prop["type"];
        type_ = MeshType::Triangles;
        
        // Referencing mesh
        mesh_ = getAsset<Mesh>(prop, "mesh");
        if (!mesh_) {
            return false;
        }

        // Create OpenGL buffer objects
        std::vector<Float> vs;
        mesh_->foreachTriangle([&](int, Vec3 p1, Vec3 p2, Vec3 p3) {
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

        init_ = true;
        return true;
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

LM_COMP_REG_IMPL(Mesh_VisGL, "mesh::visgl");

// ----------------------------------------------------------------------------

// Interactive visualizer using OpenGL
class Renderer_VisGL final : public Renderer {
private:
    Camera* camera_;
    GLResource pipeline_;
    mutable GLResource programV_;
    mutable GLResource programF_;

public:
    virtual bool construct(const Json& prop) override {
        // Camera
        camera_ = getAsset<Camera>(prop, "camera");
        if (!camera_) {
            return false;
        }

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
        if (!programV_.compileString(GL_VERTEX_SHADER, RenderVs)) return false;
        if (!programF_.compileString(GL_FRAGMENT_SHADER, RenderFs)) return false;
        if (!programV_.link()) return false;
        if (!programF_.link()) return false;
        pipeline_.create(GLResourceType::Pipeline);
        pipeline_.addProgram(programV_);
        pipeline_.addProgram(programF_);

        return true;
    }
    
    // This function is called one per frame
    virtual void render(const Scene* scene) const override {
        // State
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Camera
        const auto viewM = camera_->viewMatrix();
        const auto projM = camera_->projectionMatrix();
        programV_.setUniform("ViewMatrix", glm::mat4(viewM));
        programV_.setUniform("ProjectionMatrix", glm::mat4(projM));

        // Render meshes
        pipeline_.bind();
        scene->foreachPrimitive([&](const Primitive& primitive) {
            const auto* mesh = dynamic_cast<Mesh_VisGL*>(primitive.mesh);
            const auto* material = dynamic_cast<Material_VisGL*>(primitive.material);
            if (!mesh || !material) {
                return;
            }
            programV_.setUniform("ModelMatrix", glm::mat4(primitive.transform.M));
            programF_.setUniform("Color", glm::vec3(1));
            programF_.setUniform("Alpha", 1.f);
            programF_.setUniform("UseConstantColor", 1);
            material->apply([&]() {
                mesh->render();
            });
        });
        pipeline_.unbind();

        // Restore
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_BLEND);
    }
};

LM_COMP_REG_IMPL(Renderer_VisGL, "renderer::visgl");

LM_NAMESPACE_END(LM_NAMESPACE)
