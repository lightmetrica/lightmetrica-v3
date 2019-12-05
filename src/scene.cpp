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

// ------------------------------------------------------------------------------------------------

class Assets final : public Component {
private:
    std::vector<Component::Ptr<Component>> assets_;
    std::unordered_map<std::string, int> asset_index_map_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(asset_index_map_, assets_);
    }

    virtual void foreach_underlying(const ComponentVisitor& visitor) override {
        for (auto& asset : assets_) {
            comp::visit(visitor, asset);
        }
    }

    virtual Component* underlying(const std::string& name) const override {
        auto it = asset_index_map_.find(name);
        if (it == asset_index_map_.end()) {
            LM_ERROR("Invalid asset name [name='{}']", name);
            return nullptr;
        }
        return assets_.at(it->second).get();
    }

private:
    bool valid_asset_name(const std::string& name) const {
        std::regex regex(R"x([:\w_-]+)x");
        std::smatch match;
        return std::regex_match(name, match, regex);
    }

public:
    Component* load_asset(const std::string& name, const std::string& implKey, const Json& prop) {
        LM_INFO("Loading asset [name='{}']", name);
        LM_INDENT();

        // Check if asset name is valid
        if (!valid_asset_name(name)) {
            LM_ERROR("Invalid asset name [name='{}']", name);
            return nullptr;
        }

        // Check if the asset with given name has been already loaded
        const auto it = asset_index_map_.find(name);
        const bool found = it != asset_index_map_.end();
        if (found) {
            LM_INFO("Asset [name='{}'] has been already loaded. Replacing..", name);
        }

        // Create an instance of the asset
        auto p = comp::create_without_construct<Component>(implKey, make_loc(loc(), name));
        if (!p) {
            LM_ERROR("Failed to create an asset [name='{}', key='{}']", name, implKey);
            return nullptr;
        }

        // Register created asset
        // Note that the registration must happen before construct() because
        // the instance could be accessed by underlying() while initialization.
        Component* asset;
        if (found) {
            // Move existing instance
            // This must not be released until the end of this scope
            // because weak references needs to find locator in the existing instance.
            auto old = std::move(assets_[it->second]);

            // Replace the existing instance
            assets_[it->second] = std::move(p);
            asset = assets_[it->second].get();

            // Initialize the asset
            // This might cause an exception
            asset->construct(prop);

            // Notify to update the weak references in the object tree
            const lm::Component::ComponentVisitor visitor = [&](lm::Component*& comp, bool weak) {
                if (!comp) {
                    return;
                }
                if (asset == comp) {
                    // Ignore myself
                    return;
                }
                if (!weak) {
                    comp->foreach_underlying(visitor);
                }
                else {
                    comp::update_weak_ref(comp);
                }
            };
            comp::get<lm::Component>("$")->foreach_underlying(visitor);
        }
        else {
            // Register as a new asset
            asset_index_map_[name] = int(assets_.size());
            assets_.push_back(std::move(p));
            asset = assets_.back().get();

            // Initialize the asset
            asset->construct(prop);
        }

        return asset;
    }
};

LM_COMP_REG_IMPL(Assets, "assets::default");

// ------------------------------------------------------------------------------------------------

struct LightPrimitiveIndex {
    Transform global_transform; // Global transform matrix
    int index;                 // Primitive node index

    template <typename Archive>
    void serialize(Archive& ar) {
        ar(global_transform, index);
    }
};

class Scene_ final : public Scene {
private:
    Ptr<Assets> assets_;                             // Underlying assets
    std::vector<SceneNode> nodes_;                   // Scene nodes
    Ptr<Accel> accel_;                               // Acceleration structure
    std::optional<int> camera_;                      // Camera index
    std::vector<LightPrimitiveIndex> lights_;        // Primitive node indices of lights and global transforms
    std::unordered_map<int, int> light_indices_map_; // Map from node indices to light indices.
    std::optional<int> env_light_;                   // Environment light index
    std::optional<int> medium_;                      // Medium index

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(assets_, nodes_, accel_, camera_, lights_, light_indices_map_, env_light_);
    }

    virtual void foreach_underlying(const ComponentVisitor& visit) override {
        comp::visit(visit, assets_);
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

    virtual Component* underlying(const std::string& name) const override {
        if (name == "assets") {
            return assets_.get();
        }
        else if (name == "accel") {
            return accel_.get();
        }
        else if (name == "camera") {
            return nodes_.at(*camera_).primitive.camera;
        }
        return nullptr;
    }

public:
    virtual void construct(const Json&) override {
        // Assets
        assets_ = comp::create<Assets>("assets::default", make_loc("assets"));
        // Index 0 is fixed to the scene group
        nodes_.push_back(SceneNode::make_group(0, false, {}));
    }

public:
    virtual Component* load_asset(const std::string& name, const std::string& implKey, const Json& prop) override {
        return assets_->load_asset(name, implKey, prop);
    }

    // --------------------------------------------------------------------------------------------

    virtual int root_node() override {
        return 0;
    }

    virtual int create_node(SceneNodeType type, const Json& prop) override {
        if (type == SceneNodeType::Primitive) {
            // Find an asset by property name
            const auto get_asset_ref_by = [&](const std::string& propName) -> Component* {
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
                LM_ERROR("Invalid primitive node. Given assets are invalid.");
                return false;
            }
            if (camera && light) {
                LM_ERROR("Primitive cannot be both camera and light");
                return false;
            }

            // Camera
            if (camera) {
                camera_ = index;
            }

            // Envlight
            if (light && light->is_infinite()) {
                if (env_light_) {
                    LM_ERROR("Environment light is already registered. "
                             "You can register only one environment light in the scene.");
                    return false;
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
            
        // ----------------------------------------------------------------------------------------
        
        if (type == SceneNodeType::Group) {
            const int index = int(nodes_.size());
            nodes_.push_back(SceneNode::make_group(
                index,
                json::value<bool>(prop, "instanced", false),
                json::value_or_none<Mat4>(prop, "transform")
            ));
            return index;
        }

        // ----------------------------------------------------------------------------------------

        LM_UNREACHABLE_RETURN();
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

    // --------------------------------------------------------------------------------------------

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

    virtual int num_lights() const override {
        return (int)(lights_.size());
    }

    // --------------------------------------------------------------------------------------------

    virtual void build(const std::string& name, const Json& prop) override {
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

        // Build acceleration structure
        accel_ = comp::create<Accel>(name, make_loc(loc(), "accel"), prop);
        if (!accel_) {
            return;
        }
        LM_INFO("Building acceleration structure [name='{}']", name);
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
        const auto [t, uv, global_transform, primitiveIndex, faceIndex] = *hit;
        const auto& primitive = nodes_.at(primitiveIndex).primitive;
        const auto p = primitive.mesh->surface_point(faceIndex, uv);
        return SceneInteraction::make_surface_interaction(
            primitiveIndex,
            PointGeometry::make_on_surface(
                global_transform.M * Vec4(p.p, 1_f),
                glm::normalize(global_transform.normal_M * p.n),
                p.t
            )
        );
    }

    // --------------------------------------------------------------------------------------------

    virtual bool is_light(const SceneInteraction& sp) const override {
        const auto& primitive = nodes_.at(sp.primitive).primitive;
        return sp.medium
            ? primitive.medium->is_emitter()
            : primitive.light != nullptr;
    }

    virtual bool is_specular(const SceneInteraction& sp, int comp) const override {
        const auto& primitive = nodes_.at(sp.primitive).primitive;
        if (sp.medium) {
            return primitive.medium->phase()->is_specular(sp.geom);
        }
        if (sp.endpoint) {
            if (primitive.light) {
                return primitive.light->is_specular(sp.geom, comp);
            }
            else if (primitive.camera) {
                return primitive.camera->is_specular(sp.geom);
            }
            LM_UNREACHABLE_RETURN();
        }
        return primitive.material->is_specular(sp.geom, comp);
    }

    // --------------------------------------------------------------------------------------------

    virtual Ray primary_ray(Vec2 rp, Float aspect_ratio) const {
        return nodes_.at(*camera_).primitive.camera->primary_ray(rp, aspect_ratio);
    }

    virtual std::optional<RaySample> sample_ray(Rng& rng, const SceneInteraction& sp, Vec3 wi) const override {
        if (sp.medium) {
            // Medium interaction
            const auto& primitive = nodes_.at(sp.primitive).primitive;
            const auto s = primitive.medium->phase()->sample(rng, sp.geom, wi);
            if (!s) {
                return {};
            }
            return RaySample{
                sp,
                0,
                s->wo,
                s->weight
            };
        }
        else if (sp.terminator && sp.terminator == TerminatorType::Camera) {
            // Endpoint
            const auto* camera = nodes_.at(*camera_).primitive.camera;
            const auto s = camera->sample_primary_ray(rng, sp.cameraCond.window, sp.cameraCond.aspect_ratio);
            if (!s) {
                return {};
            }
            return RaySample{
                SceneInteraction::make_camera_endpoint(
                    *camera_,
                    s->geom,
                    sp.cameraCond.window,
                    sp.cameraCond.aspect_ratio
                ),
                0,
                s->wo,
                s->weight
            };
        }
        else {
            // Surface interaction
            const auto& primitive = nodes_.at(sp.primitive).primitive;
            if (!primitive.material) {
                return {};
            }
            const auto s = primitive.material->sample(rng, sp.geom, wi);
            if (!s) {
                return {};
            }
            return RaySample{
                SceneInteraction::make_surface_interaction(
                    sp.primitive,
                    sp.geom
                ),
                s->comp,
                s->wo,
                s->weight
            };
        }
    }

    virtual std::optional<Vec3> sample_direction_given_comp(Rng& rng, const SceneInteraction& sp, int comp, Vec3 wi) const  override {
        // Currently only the surface interaction is supported
        if (sp.medium || sp.terminator) {
            LM_THROW_EXCEPTION_DEFAULT(Error::Unsupported);
        }

        const auto& primitive = nodes_.at(sp.primitive).primitive;
        if (!primitive.material) {
            return {};
        }
        return primitive.material->sample_direction_given_comp(rng, sp.geom, comp, wi);
    }

    virtual Float pdf_comp(const SceneInteraction& sp, int comp, Vec3 wi) const override {
        const auto& primitive = nodes_.at(sp.primitive).primitive;
        if (sp.medium) {
            return 1_f;
        }
        else {
            if (!primitive.material) {
                return 1_f;
            }
            return primitive.material->pdf_comp(sp.geom, comp, wi);
        }
    }

    virtual std::optional<Vec2> raster_position(Vec3 wo, Float aspect_ratio) const override {
        const auto* camera = nodes_.at(*camera_).primitive.camera;
        return camera->raster_position(wo, aspect_ratio);
    }

    virtual std::optional<RaySample> sample_direct_light(Rng& rng, const SceneInteraction& sp) const override {
        // Sample a light
        const int n  = int(lights_.size());
        const int i  = glm::clamp(int(rng.u() * n), 0, n-1);
        const auto pL = 1_f / n;
        
        // Sample a position on the light
        const auto light = lights_.at(i);
        const auto& primitive = nodes_.at(light.index).primitive;
        const auto s = primitive.light->sample(rng, sp.geom, light.global_transform);
        if (!s) {
            return {};
        }
        return RaySample{
            SceneInteraction::make_light_endpoint(
                light.index,
                s->geom
            ),
            s->comp,
            s->wo,
            s->weight / pL
        };
    }
    
    virtual Float pdf(const SceneInteraction& sp, int comp, Vec3 wi, Vec3 wo) const override {
        const auto& primitive = nodes_.at(sp.primitive).primitive;
        if (sp.medium) {
            return primitive.medium->phase()->pdf(sp.geom, wi, wo);
        }
        else if (sp.endpoint) {
            if (primitive.light) {
                LM_THROW_EXCEPTION_DEFAULT(Error::Unimplemented);
            }
            else if (primitive.camera) {
                return primitive.camera->pdf(wo, sp.cameraCond.aspect_ratio);
            }
            LM_UNREACHABLE_RETURN();
        }
        else {
            return primitive.material->pdf(sp.geom, comp, wi, wo);
        }
    }

    virtual Float pdf_direct_light(const SceneInteraction& sp, const SceneInteraction& spL, int compL, Vec3 wo) const override {
        const auto& primitive = nodes_.at(spL.primitive).primitive;
        const auto light_transform = lights_.at(light_indices_map_.at(spL.primitive)).global_transform;
        const auto pL = 1_f / int(lights_.size());
        return primitive.light->pdf(sp.geom, spL.geom, compL, light_transform, wo) * pL;
    }

    // --------------------------------------------------------------------------------------------

    virtual std::optional<DistanceSample> sample_distance(Rng& rng, const SceneInteraction& sp, Vec3 wo) const override {
        // Intersection to next surface
        const auto hit = intersect({ sp.geom.p, wo }, Eps, Inf);
        const auto dist = hit && !hit->geom.infinite ? glm::length(hit->geom.p - sp.geom.p) : Inf;

        // Sample a distance
        const auto* medium = nodes_.at(*medium_).primitive.medium;
        const auto ds = medium->sample_distance(rng, { sp.geom.p, wo }, 0_f, dist);
        if (ds && ds->medium) {
            // Medium interaction
            return DistanceSample{
                SceneInteraction::make_medium_interaction(
                    *medium_,
                    PointGeometry::make_degenerated(ds->p)
                ),
                ds->weight
            };
        }
        else {
            // Surface interaction
            return DistanceSample{
                *hit,
                ds ? ds->weight : Vec3(1_f)
            };
        }
    }

    virtual Vec3 eval_transmittance(Rng& rng, const SceneInteraction& sp1, const SceneInteraction& sp2) const override {
        if (!visible(sp1, sp2)) {
            return Vec3(0_f);
        }
        if (!medium_) {
            return Vec3(1_f);
        }
        
        // Extended distance between two points
        assert(!sp1.geom.infinite);
        const auto dist = !sp2.geom.infinite
            ? glm::distance(sp1.geom.p, sp2.geom.p)
            : Inf;
        const auto wo = !sp2.geom.infinite
            ? glm::normalize(sp2.geom.p - sp1.geom.p)
            : -sp2.geom.wo;

        const auto* medium = nodes_.at(*medium_).primitive.medium;
        return medium->eval_transmittance(rng, { sp1.geom.p, wo }, 0_f, dist);
    }

    // --------------------------------------------------------------------------------------------

    virtual Vec3 eval_contrb(const SceneInteraction& sp, int comp, Vec3 wi, Vec3 wo) const override {
        const auto& primitive = nodes_.at(sp.primitive).primitive;
        if (sp.medium) {
            // Medium interaction
            return primitive.medium->phase()->eval(sp.geom, wi, wo);
        }
        else {
            // Surface interaction
            if (sp.endpoint) {
                if (primitive.camera) {
                    return primitive.camera->eval(wo, sp.cameraCond.aspect_ratio);
                }
                else if (primitive.light) {
                    return primitive.light->eval(sp.geom, comp, wo);
                }
                LM_UNREACHABLE();
            }
            return primitive.material->eval(sp.geom, comp, wi, wo);
        }
    }

    virtual Vec3 eval_contrb_endpoint(const SceneInteraction& sp, Vec3 wo) const override {
        const auto& primitive = nodes_.at(sp.primitive).primitive;
        if (!primitive.light) {
            return {};
        }
        return primitive.light->eval(sp.geom, -1, wo);
    }

    virtual std::optional<Vec3> reflectance(const SceneInteraction& sp, int comp) const override {
        const auto& primitive = nodes_.at(sp.primitive).primitive;
        if (!primitive.material) {
            return {};
        }
        return primitive.material->reflectance(sp.geom, comp);
    }
};

LM_COMP_REG_IMPL(Scene_, "scene::default");

LM_NAMESPACE_END(LM_NAMESPACE)
