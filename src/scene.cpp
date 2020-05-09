/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/scene.h>
#include <lm/accel.h>
#include <lm/mesh.h>
#include <lm/camera.h>
#include <lm/material.h>
#include <lm/light.h>
#include <lm/model.h>
#include <lm/medium.h>
#include <lm/phase.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Scene_ final : public Scene {
private:
    Accel* accel_;                                   // Acceleration structure
    std::vector<SceneNode> nodes_;                   // Scene nodes (index 0: root node)
    std::optional<int> camera_;                      // Camera index
    std::vector<LightPrimitiveIndex> lights_;        // Primitive node indices of lights and global transforms
    std::unordered_map<int, int> light_indices_map_; // Map from node indices to light indices.
    std::optional<int> env_light_;                   // Environment light index
    std::optional<int> medium_;                      // Medium index

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(accel_, nodes_, camera_, lights_, light_indices_map_, env_light_);
    }

    virtual void foreach_underlying(const ComponentVisitor& visit) override {
        comp::visit(visit, accel_);
        for (auto& node : nodes_) {
            if (node.type == SceneNodeType::Primitive) {
                comp::visit(visit, node.primitive.mesh);
                comp::visit(visit, node.primitive.material);
                comp::visit(visit, node.primitive.light);
                comp::visit(visit, node.primitive.camera);
            }
        }
    }

public:
    virtual void construct(const Json& prop) override {
        accel_ = json::comp_ref_or_nullptr<Accel>(prop, "accel");
        reset();
    }

public:
    virtual void reset() override {
        nodes_.clear();
        camera_ = {};
        lights_.clear();
        light_indices_map_.clear();
        env_light_ = {};
        medium_ = {};
        nodes_.push_back(SceneNode::make_group(0, false, {}));
    }

    // --------------------------------------------------------------------------------------------

    #pragma region Scene graph manipulation and access

    virtual int root_node() override {
        return 0;
    }

    virtual int create_primitive_node(const Json& prop) override {
        // Find an asset by property name
        const auto get_asset_ref_by = [&](const std::string& propName) -> Component * {
            const auto it = prop.find(propName);
            if (it == prop.end()) {
                return nullptr;
            }
            return comp::get<Component>(it.value().get<std::string>());
        };

        // Node index
        const int index = int(nodes_.size());

        // Get asset references
        auto* mesh = dynamic_cast<Mesh*>(get_asset_ref_by("mesh"));
        auto* material = dynamic_cast<Material*>(get_asset_ref_by("material"));
        auto* light = dynamic_cast<Light*>(get_asset_ref_by("light"));
        auto* camera = dynamic_cast<Camera*>(get_asset_ref_by("camera"));
        auto* medium = dynamic_cast<Medium*>(get_asset_ref_by("medium"));

        // Check validity
        if (!mesh && !material && !light && !camera && !medium) {
            LM_THROW_EXCEPTION(Error::InvalidArgument, "Invalid primitive node. Given assets are invalid.");
        }
        if (camera && light) {
            LM_THROW_EXCEPTION(Error::InvalidArgument, "Primitive cannot be both camera and light.");
        }

        // If you specify the mesh, you also need to specify the material
        if ((!mesh && material) || (mesh && !material)) {
            LM_THROW_EXCEPTION(Error::InvalidArgument, "You must specify both mesh and material.");
        }

        // Camera
        if (camera) {
            camera_ = index;
        }

        // Envlight
        if (light && light->is_env()) {
            if (env_light_) {
                LM_THROW_EXCEPTION(Error::InvalidArgument, "Environment light is already registered. "
                    "You can register only one environment light in the scene.");
            }
            env_light_ = index;
        }

        // Medium
        if (medium) {
            // For now, consider the medium as global asset.
            medium_ = index;
        }

        // Create primitive node
        nodes_.push_back(SceneNode::make_primitive(index, mesh, material, light, camera, medium));

        return index;
    }

    virtual int create_group_node(Mat4 transform) override {
        const int index = int(nodes_.size());
        nodes_.push_back(SceneNode::make_group(index, false, transform));
        return index;
    }

    virtual int create_instance_group_node() override {
        const int index = int(nodes_.size());
        nodes_.push_back(SceneNode::make_group(index, true, {}));
        return index;
    }

    virtual void add_child(int parent, int child) override {
        if (parent < 0 || parent >= int(nodes_.size())) {
            LM_ERROR("Missing parent index [index='{}'", parent);
            return;
        }

        auto& node = nodes_.at(parent);
        if (node.type != SceneNodeType::Group) {
            LM_ERROR("Adding child to non-group node [parent='{}', child='{}']", parent, child);
            return;
        }

        node.group.children.push_back(child);
    }

    virtual void add_child_from_model(int parent, const std::string& modelLoc) override {
        if (parent < 0 || parent >= int(nodes_.size())) {
            LM_ERROR("Missing parent index [index='{}'", parent);
            return;
        }

        auto* model = comp::get<Model>(modelLoc);
        if (!model) {
            return;
        }

        model->create_primitives([&](Component* mesh, Component* material, Component* light) {
            const int index = int(nodes_.size());
            nodes_.push_back(SceneNode::make_primitive(
                index,
                dynamic_cast<Mesh*>(mesh),
                dynamic_cast<Material*>(material),
                dynamic_cast<Light*>(light),
                nullptr,
                nullptr));
            add_child(parent, index);
        });
    }

    virtual int create_group_from_model(const std::string& modelLoc) override {
        auto* model = comp::get<Model>(modelLoc);
        if (!model) {
            LM_ERROR("Invalid model [loc={}]", modelLoc);
            return -1;
        }

        // Iterate underlyng scene nodes in the model
        // To copy the underlying scene graph as a child of scene graph of the scene,
        // we first want to copy the nodes and modify the references to the other nodes.
        // Actually, we merely want to offset the indices by nodes_.size().
        const int offset = int(nodes_.size());
        model->foreach_node([&](const SceneNode& modelNode) {
            // Copy the node
            nodes_.push_back(modelNode);
            auto& node = nodes_.back();

            // Update the references to other nodes
            node.index += offset;
            if (node.type == SceneNodeType::Group) {
                for (int& child : node.group.children) {
                    child += offset;
                }
            }
            else if (node.type == SceneNodeType::Primitive) {
                if (node.primitive.camera) {
                    camera_ = node.index;
                }
            }
        });

        return offset;
    }

    virtual void traverse_primitive_nodes(const NodeTraverseFunc& traverseFunc) const override {
        std::function<void(int, Mat4)> visit = [&](int index, Mat4 global_transform) {
            const auto& node = nodes_.at(index);
            traverseFunc(node, global_transform);
            if (node.type == SceneNodeType::Group) {
                const auto M = node.group.local_transform
                    ? global_transform * *node.group.local_transform
                    : global_transform;
                for (int child : node.group.children) {
                    visit(child, M);
                }
            }
        };
        visit(0, Mat4(1_f));
    }

    virtual void visit_node(int node_index, const VisitNodeFunc& visit) const override {
        visit(nodes_.at(node_index));
    }

    virtual const SceneNode& node_at(int node_index) const override {
        return nodes_.at(node_index);
    }

    virtual int num_nodes() const override {
        return (int)(nodes_.size());
    }

    virtual int env_light_node() const override {
        return env_light_ ? *env_light_ : -1;
    }

    virtual int camera_node() const override {
        return camera_ ? *camera_ : -1;
    }

    virtual int medium_node() const override {
        return medium_ ? *medium_ : -1;
    }

    virtual int num_lights() const override {
        return (int)(lights_.size());
    }

    #pragma endregion

    // --------------------------------------------------------------------------------------------

    #pragma region Ray-scene intersection

    virtual Accel* accel() const override {
        return accel_;
    }

    virtual void set_accel(const std::string& accel_loc) override {
        accel_ = comp::get<Accel>(accel_loc);
    }

    virtual void build() override {
        // Update light indices
        // We keep the global transformation of the light primitive as well as the references.
        // We need to recompute the indices when an update of the scene happens,
        // because the global tranformation can only be obtained by traversing the nodes.
        light_indices_map_.clear();
        lights_.clear();
        traverse_primitive_nodes([&](const SceneNode& node, Mat4 global_transform) {
            if (node.type == SceneNodeType::Primitive && node.primitive.light) {
                light_indices_map_[node.index] = int(lights_.size());
                lights_.push_back({ Transform(global_transform), node.index });
            }
        });

        // Compute scene bound
        Bound bound;
        traverse_primitive_nodes([&](const SceneNode& node, Mat4 global_transform) {
            if (node.type != SceneNodeType::Primitive) {
                return;
            }
            if (!node.primitive.mesh) {
                return;
            }
            node.primitive.mesh->foreach_triangle([&](int, const Mesh::Tri& tri) {
                const auto p1 = global_transform * Vec4(tri.p1.p, 1_f);
                const auto p2 = global_transform * Vec4(tri.p2.p, 1_f);
                const auto p3 = global_transform * Vec4(tri.p3.p, 1_f);
                bound = merge(bound, p1);
                bound = merge(bound, p2);
                bound = merge(bound, p3);
            });
        });
        
        // Set scene bound to the lights
        for (auto& l : lights_) {
            auto* light = nodes_.at(l.index).primitive.light;
            light->set_scene_bound(bound);
        }

        // Build acceleration structure
        LM_INFO("Building acceleration structure [name='{}']", accel_->name());
        LM_INDENT();
        accel_->build(*this);
    }

    virtual std::optional<SceneInteraction> intersect(Ray ray, Float tmin, Float tmax) const override {
        const auto hit = accel_->intersect(ray, tmin, tmax);
        if (!hit) {
            // Use environment light when tmax = Inf
            if (tmax < Inf) {
                return {};
            }
            if (!env_light_) {
                return {};
            }
            return SceneInteraction::make_light_endpoint(
                *env_light_,
                PointGeometry::make_infinite(-ray.d));
        }
        const auto[t, uv, global_transform, primitiveIndex, face_index] = *hit;
        const auto& primitive = nodes_.at(primitiveIndex).primitive;
        const auto p = primitive.mesh->surface_point(face_index, uv);
        return SceneInteraction::make_surface_interaction(
            primitiveIndex,
            PointGeometry::make_on_surface(
                global_transform.M * Vec4(p.p, 1_f),
                glm::normalize(global_transform.normal_M * p.n),
                glm::normalize(global_transform.normal_M * p.gn),
                p.t
            )
        );
    }

    #pragma endregion

    // --------------------------------------------------------------------------------------------

    #pragma region Primitive type checking

    virtual bool is_light(const SceneInteraction& sp) const override {
        const auto& primitive = nodes_.at(sp.primitive).primitive;
        if (sp.is_type(SceneInteraction::MediumInteraction)) {
            return primitive.medium->is_emitter();
        }
        else {
            // Note: SurfaceInterfaction can contain light component.
            return primitive.light != nullptr;
        }
    }

    virtual bool is_camera(const SceneInteraction& sp) const override {
        return sp.primitive == *camera_;
    }

    #pragma endregion

    // --------------------------------------------------------------------------------------------

    #pragma region Light sampling

    virtual LightSelectionSample sample_light_selection(Float u) const override {
        const int n = int(lights_.size());
        const int i = glm::clamp(int(u * n), 0, n - 1);
        const auto pL = 1_f / n;
        return LightSelectionSample{
            i,
            pL
        };
    }

    virtual Float pdf_light_selection(int) const override {
        const int n = int(lights_.size());
        return 1_f / n;
    };

    virtual LightPrimitiveIndex light_primitive_index_at(int light_index) const override {
        return lights_.at(light_index);
    }

    virtual int light_index_at(int node_index) const override {
        return light_indices_map_.at(node_index);
    }

    #pragma endregion
};

LM_COMP_REG_IMPL(Scene_, "scene::default");

LM_NAMESPACE_END(LM_NAMESPACE)
