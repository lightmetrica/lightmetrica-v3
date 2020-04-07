#include <lm/lm.h>
#pragma warning(push)
#pragma warning(disable:4244)
#undef _CRT_SECURE_NO_WARNINGS
#include <pbrtParser/Scene.h>
#pragma warning(pop)

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// ------------------------------------------------------------------------------------------------

namespace {

Vec3 convert_pbrt_vec3(const pbrt::vec3f& v) {
    return Vec3(v.x, v.y, v.z);
}

Vec2 convertPbrtVec2(const pbrt::vec2f& v) {
    return Vec2(v.x, v.y);
}

Mat3 convertPbrtMat3(const pbrt::math::mat3f& m) {
    return Mat3(
        convert_pbrt_vec3(m.vx),
        convert_pbrt_vec3(m.vy),
        convert_pbrt_vec3(m.vz));
}

Mat4 convertPbrtXfm(const pbrt::affine3f& v) {
    const auto rotM = Mat4(convertPbrtMat3(v.l));
    const auto transM = glm::translate(convert_pbrt_vec3(v.p));
    return transM * rotM;
}

}

// ------------------------------------------------------------------------------------------------

class Mesh_PBRT : public Mesh {
private:
    pbrt::TriangleMesh* pbrt_mesh_;

public:
    virtual void construct(const Json& prop) override {
        pbrt_mesh_ = prop["mesh_"].get<pbrt::TriangleMesh*>();
        assert(pbrt_mesh_);
    }

    virtual void foreach_triangle(const ProcessTriangleFunc& process_triangle) const override {
        for (int fi = 0; fi < int(pbrt_mesh_->index.size()); fi++) {
            process_triangle(fi, triangle_at(fi));
        }
    }

    virtual Tri triangle_at(int face) const override {
        const auto pointByFace = [&](int fi) -> Point {
            return Point{
                convert_pbrt_vec3(pbrt_mesh_->vertex[fi]),
                pbrt_mesh_->normal.empty()
                    ? Vec3()
                    : convert_pbrt_vec3(pbrt_mesh_->normal[fi]),
                pbrt_mesh_->texcoord.empty()
                    ? Vec3()
                    : convertPbrtVec2(pbrt_mesh_->texcoord[fi])
            };
        };
        const auto index = pbrt_mesh_->index[face];
        return Tri{
            pointByFace(index.x),
            pointByFace(index.y),
            pointByFace(index.z)
        };
    }

    virtual InterpolatedPoint surface_point(int face, Vec2 uv) const override {
        const auto index = pbrt_mesh_->index[face];
        const auto p1 = convert_pbrt_vec3(pbrt_mesh_->vertex[index.x]);
        const auto p2 = convert_pbrt_vec3(pbrt_mesh_->vertex[index.y]);
        const auto p3 = convert_pbrt_vec3(pbrt_mesh_->vertex[index.z]);
        InterpolatedPoint p;
        // Position
        p.p = math::mix_barycentric(p1, p2, p3, uv);
        // Normal
        p.gn = glm::normalize(glm::cross(p2 - p1, p3 - p1));
        p.n = pbrt_mesh_->normal.empty()
            ? p.gn
            : glm::normalize(math::mix_barycentric(
                convert_pbrt_vec3(pbrt_mesh_->normal[index.x]),
                convert_pbrt_vec3(pbrt_mesh_->normal[index.y]),
                convert_pbrt_vec3(pbrt_mesh_->normal[index.z]), uv));
        // Texture coordinates
        p.t = pbrt_mesh_->texcoord.empty()
            ? Vec3()
            : math::mix_barycentric(
                convertPbrtVec2(pbrt_mesh_->texcoord[index.x]),
                convertPbrtVec2(pbrt_mesh_->texcoord[index.y]),
                convertPbrtVec2(pbrt_mesh_->texcoord[index.z]), uv);
        return p;
    }

    virtual int num_triangles() const override {
        return int(pbrt_mesh_->index.size());
    }
};

LM_COMP_REG_IMPL(Mesh_PBRT, "mesh::pbrt");

// ------------------------------------------------------------------------------------------------

class Model_PBRT : public Model {
private:
    pbrt::Scene::SP pbrt_scene_;		// Scene of PBRT parser
    std::vector<Ptr<Mesh>> meshes_;		// Underlying meshes
    Ptr<Camera> camera_;				// Underlying camera
    Ptr<Material> defaultMaterial_;		// Default material (TODO: replace after material support)
    std::vector<SceneNode> nodes_;		// Scene nodes
    
public:
    Model_PBRT() {
        // Index 0 is fixed to the scene group
        nodes_.push_back(SceneNode::make_group(0, false, {}));
    }

public:
    virtual void construct(const Json& prop) override {
        // Load PBRT scene
        const std::string path = json::value<std::string>(prop, "path");
        pbrt_scene_ = pbrt::importPBRT(path);
        if (!pbrt_scene_) {
            LM_THROW_EXCEPTION(Error::IOError, "Failed to load PBRT scene [path='{}']", path);
        }

        // Load camera
        if (!pbrt_scene_->cameras.empty()) {
            // View matrix
            const auto pbrtCamera = pbrt_scene_->cameras[0];
            auto viewM = convertPbrtXfm(pbrtCamera->frame);

            // Create camera asset
            camera_ = lm::comp::create<Camera>("camera::pinhole", make_loc("camera"), {
                {"matrix", viewM},
                {"vfov", Float(pbrtCamera->fov)}
            });
            if (!camera_) {
                LM_THROW_EXCEPTION(Error::InvalidArgument, "Failed to create camera");
            }

            // Add camera primitive node
            const int index = int(nodes_.size());
            nodes_.push_back(SceneNode::make_primitive(
                index,
                nullptr,
                nullptr,
                nullptr,
                camera_.get(),
                nullptr
            ));
            nodes_[0].group.children.push_back(index);
        }

        // Create default material
        defaultMaterial_ = lm::comp::create<Material>("material::diffuse", make_loc("defautMaterial"), {
            {"Kd", Vec3(1,1,1)}
        });

        // Load meshes
        int meshCount = 0;
        std::unordered_map<std::string, int> visited;	// Name of PBRT object -> node index
        using VisitObjectFunc = std::function<void(int, pbrt::Object::SP, const pbrt::affine3f&)>;
        const VisitObjectFunc visit_object = [&](int parent, pbrt::Object::SP pbrt_object, const pbrt::affine3f& instance_xfm) {
            // Process the underlying shapes
            if (!pbrt_object->shapes.empty()) {
                // For each shape in the pbrt object
                for (auto& shape : pbrt_object->shapes) {
                    // Create a mesh
                    auto mesh = std::dynamic_pointer_cast<pbrt::TriangleMesh>(shape);
                    if (!mesh) {
                        continue;
                    }
                    auto lmMesh = lm::comp::create<Mesh>("mesh::pbrt", make_loc(std::to_string(meshCount++)), {
                        {"mesh_", mesh.get()}
                    });
                    assert(lmMesh);
                    meshes_.push_back(std::move(lmMesh));

                    // Create a scene node
                    const int index = int(nodes_.size());
                    nodes_.push_back(SceneNode::make_primitive(
                        index,
                        meshes_.back().get(),
                        defaultMaterial_.get(),
                        nullptr,
                        nullptr,
                        nullptr
                    ));
                    nodes_[parent].group.children.push_back(index);
                }
            }

            // Process the instanced objects
            if (!pbrt_object->instances.empty()) {
                for (auto inst : pbrt_object->instances) {
                    // Global transform
                    const auto global_xfm = instance_xfm * inst->xfm;

                    // Create a transform group
                    const int transform_group_index = int(nodes_.size());
                    nodes_.push_back(SceneNode::make_group(
                        transform_group_index,
                        false,
                        convertPbrtXfm(global_xfm)
                    ));
                    auto& transformGroup = nodes_.back();
                    nodes_[parent].group.children.push_back(transform_group_index);

                    // If we already created the instance group, use it
                    const auto it = visited.find(inst->object->name);
                    if (it != visited.end()) {
                        transformGroup.group.children.push_back(it->second);
                    }
                    // Otherwise, recursively process the child node
                    else {
                        // Create instance group node
                        const int instance_group_node_index = int(nodes_.size());
                        nodes_.push_back(SceneNode::make_group(
                            instance_group_node_index,
                            true,
                            Mat4(1_f)
                        ));
						visit_object(instance_group_node_index, inst->object, global_xfm);
                        visited[inst->object->name] = instance_group_node_index;
                    }
                }
            }
        };
        visit_object(0, pbrt_scene_->world, pbrt::affine3f::identity());
    }

    virtual void create_primitives(const CreatePrimitiveFunc& createPrimitive) const override {
        for (const auto& mesh : meshes_) {
            createPrimitive(mesh.get(), defaultMaterial_.get(), nullptr);
        }
    }

    virtual void foreach_node(const VisitNodeFuncType& visit) const override {
        for (const auto& node : nodes_) {
            visit(node);
        }
    }
};

LM_COMP_REG_IMPL(Model_PBRT, "model::pbrt");

LM_NAMESPACE_END(LM_NAMESPACE)
