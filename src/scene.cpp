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

struct LightPrimitiveIndex {
    Transform global_transform;  // Global transform matrix
    int index;                   // Primitive node index

    template <typename Archive>
    void serialize(Archive& ar) {
        ar(global_transform, index);
    }
};

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

    //
    // Scene graph manipulation and access
    //

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

    virtual int num_lights() const override {
        return (int)(lights_.size());
    }

    // --------------------------------------------------------------------------------------------

    //
    // Ray-scene intersection
    //

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

    // --------------------------------------------------------------------------------------------

    //
    // Primitive type checking
    //

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

    // --------------------------------------------------------------------------------------------

    // Helper function for light selection

    struct LightSelectionSample {
        int light_index;    // Sampled light index
        Float p_sel;        // Selection probability
    };
    
    // Light selection sampling
    LightSelectionSample sample_light_selection(Rng& rng) const {
        const int n = int(lights_.size());
        const int i = glm::clamp(int(rng.u() * n), 0, n - 1);
        const auto pL = 1_f / n;
        return LightSelectionSample{
            i,
            pL
        };
    }

    // PMF for light selection sampling
    Float pdf_light_selection(int) const {
        const int n = int(lights_.size());
        return 1_f / n;
    };

    // --------------------------------------------------------------------------------------------

    //
    // Ray sampling
    //

    virtual Ray primary_ray(Vec2 rp, Float aspect) const {
        return nodes_.at(*camera_).primitive.camera->primary_ray(rp, aspect);
    }

    virtual std::optional<RaySample> sample_ray(Rng& rng, const SceneInteraction& sp, Vec3 wi, TransDir trans_dir) const override {
        if (sp.is_type(SceneInteraction::CameraTerminator)) {
            const auto* camera = nodes_.at(*camera_).primitive.camera;
            const auto s = camera->sample_ray(rng, sp.camera_cond.window, sp.camera_cond.aspect);
            if (!s) {
                return {};
            }
            return RaySample{
                SceneInteraction::make_camera_endpoint(
                    *camera_,
                    s->geom,
                    sp.camera_cond.aspect
                ),
                s->wo,
                s->weight,
                s->specular
            };
        }
        else if (sp.is_type(SceneInteraction::LightTerminator)) {
            const auto [i, p_sel] = sample_light_selection(rng);
            const auto light_index = lights_.at(i);
            const auto* light = nodes_.at(light_index.index).primitive.light;
            const auto s = light->sample_ray(rng, light_index.global_transform);
            if (!s) {
                return {};
            }
            return RaySample{
                SceneInteraction::make_light_endpoint(
                    light_index.index,
                    s->geom
                ),
                s->wo,
                s->weight,
                s->specular
            };
        }
        else if (sp.is_type(SceneInteraction::MediumInteraction)) {
            const auto& primitive = nodes_.at(sp.primitive).primitive;
            const auto s = primitive.medium->phase()->sample_direction(rng, sp.geom, wi);
            if (!s) {
                return {};
            }
            return RaySample{
                sp,
                s->wo,
                s->weight,
                s->specular
            };
        }
        else if (sp.is_type(SceneInteraction::SurfaceInteraction)) {
            const auto& primitive = nodes_.at(sp.primitive).primitive;
            if (!primitive.material) {
                return {};
            }
            const auto s = primitive.material->sample_direction(rng, sp.geom, wi, (MaterialTransDir)(trans_dir));
            if (!s) {
                return {};
            }
            const auto sn_corr = surface::shading_normal_correction(sp.geom, wi, s->wo, trans_dir);
            return RaySample{
                SceneInteraction::make_surface_interaction(
                    sp.primitive,
                    sp.geom
                ),
                s->wo,
                s->weight * sn_corr,
                s->specular
            };
        }
        LM_UNREACHABLE_RETURN();
    }

    // --------------------------------------------------------------------------------------------

    //
    // Direction sampling
    //

    virtual std::optional<DirectionSample> sample_direction(Rng& rng, const SceneInteraction& sp, Vec3 wi, TransDir trans_dir) const override {
        if (!sp.is_type(SceneInteraction::Midpoint)) {
            LM_THROW_EXCEPTION(Error::Unsupported,
                "Direction sampling is only supported for midpoint interactions.");
        }
        const auto& primitive = nodes_.at(sp.primitive).primitive;
        if (sp.is_type(SceneInteraction::MediumInteraction)) {
            const auto s = primitive.medium->phase()->sample_direction(rng, sp.geom, wi);
            if (!s) {
                return {};
            }
            return DirectionSample{
                s->wo,
                s->weight,
                s->specular
            };
        }
        else if (sp.is_type(SceneInteraction::SurfaceInteraction)) {
            const auto s = primitive.material->sample_direction(rng, sp.geom, wi, static_cast<MaterialTransDir>(trans_dir));
            if (!s) {
                return {};
            }
            const auto sn_corr = surface::shading_normal_correction(sp.geom, wi, s->wo, trans_dir);
            return DirectionSample{
                s->wo,
                s->weight * sn_corr,
                s->specular
            };
        }
        LM_UNREACHABLE_RETURN();
    }

    virtual Float pdf_direction(const SceneInteraction& sp, Vec3 wi, Vec3 wo, bool eval_delta) const override {
        const auto& primitive = nodes_.at(sp.primitive).primitive;
        switch (sp.type) {
            case SceneInteraction::CameraEndpoint:
                return primitive.camera->pdf_direction(wo, sp.camera_cond.aspect);
            case SceneInteraction::LightEndpoint:
                LM_THROW_EXCEPTION_DEFAULT(Error::Unimplemented);
            case SceneInteraction::MediumInteraction:
                return primitive.medium->phase()->pdf_direction(sp.geom, wi, wo);
            case SceneInteraction::SurfaceInteraction:
                return primitive.material->pdf_direction(sp.geom, wi, wo, eval_delta);
        }
        LM_UNREACHABLE_RETURN();
    }

    virtual Float pdf_position(const SceneInteraction& sp) const override {
        if (!sp.is_type(SceneInteraction::Endpoint)) {
            LM_THROW_EXCEPTION(Error::Unsupported,
                "pdf_position() does not support non-endpoint interactions.");
        }
        const auto& primitive = nodes_.at(sp.primitive).primitive;
        if (sp.is_type(SceneInteraction::CameraEndpoint)) {
            return primitive.camera->pdf_position(sp.geom);
        }
        else if (sp.is_type(SceneInteraction::LightEndpoint)) {
            const int light_index = light_indices_map_.at(sp.primitive);
            const auto light_transform = lights_.at(light_index).global_transform;
            const auto pL = 1_f / int(lights_.size());
            return primitive.light->pdf_position(sp.geom, light_transform) * pL;
        }
        LM_UNREACHABLE_RETURN();
    }

    // --------------------------------------------------------------------------------------------

    //
    // Direct endpoint sampling
    //

    virtual std::optional<RaySample> sample_direct_light(Rng& rng, const SceneInteraction& sp) const override {
        // Sample a light
        const auto [light_index, p_sel] = sample_light_selection(rng);

        // Sample a position on the light
        const auto light = lights_.at(light_index);
        const auto& primitive = nodes_.at(light.index).primitive;
        const auto s = primitive.light->sample_direct(rng, sp.geom, light.global_transform);
        if (!s) {
            return {};
        }
        return RaySample{
            SceneInteraction::make_light_endpoint(
                light.index,
                s->geom
            ),
            s->wo,
            s->weight / p_sel,
            s->specular
        };
    }

    virtual std::optional<RaySample> sample_direct_camera(Rng& rng, const SceneInteraction& sp, Float aspect) const override {
        const auto& primitive = nodes_.at(*camera_).primitive;
        const auto s = primitive.camera->sample_direct(rng, sp.geom, aspect);
        if (!s) {
            return {};
        }
        return RaySample{
            SceneInteraction::make_camera_endpoint(
                *camera_,
                s->geom,
                aspect
            ),
            s->wo,
            s->weight,
            s->specular
        };
    }

    virtual Float pdf_direct(const SceneInteraction& sp, const SceneInteraction& sp_endpoint, Vec3 wo) const override {
        if (!sp_endpoint.is_type(SceneInteraction::Endpoint)) {
            LM_THROW_EXCEPTION(Error::Unsupported,
                "pdf_direct() does not support non-endpoint interactions.");
        }

        const auto& primitive = nodes_.at(sp_endpoint.primitive).primitive;
        if (sp_endpoint.is_type(SceneInteraction::CameraEndpoint)) {
            return primitive.camera->pdf_direct(sp.geom, sp_endpoint.geom, wo);
        }
        else if (sp_endpoint.is_type(SceneInteraction::LightEndpoint)) {
            const int light_index = light_indices_map_.at(sp_endpoint.primitive);
            const auto light_transform = lights_.at(light_index).global_transform;
            const auto pL = 1_f / int(lights_.size());
            return primitive.light->pdf_direct(sp.geom, sp_endpoint.geom, light_transform, wo) * pL;
        }

        LM_UNREACHABLE_RETURN();
    }

    // --------------------------------------------------------------------------------------------

    //
    // Distance sampling
    //

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

    //
    // Evaluating contribution
    //

    virtual std::optional<Vec2> raster_position(Vec3 wo, Float aspect) const override {
        const auto* camera = nodes_.at(*camera_).primitive.camera;
        return camera->raster_position(wo, aspect);
    }

    virtual Vec3 eval_contrb(const SceneInteraction& sp, Vec3 wi, Vec3 wo, TransDir trans_dir, bool eval_delta) const override {
        const auto& primitive = nodes_.at(sp.primitive).primitive;
        switch (sp.type) {
            case SceneInteraction::CameraEndpoint:
                return primitive.camera->eval(wo, sp.camera_cond.aspect);
            case SceneInteraction::LightEndpoint:
                return primitive.light->eval(sp.geom, wo);
            case SceneInteraction::MediumInteraction:
                return primitive.medium->phase()->eval(sp.geom, wi, wo);
            case SceneInteraction::SurfaceInteraction:
                return primitive.material->eval(sp.geom, wi, wo, (MaterialTransDir)(trans_dir), eval_delta) *
                       surface::shading_normal_correction(sp.geom, wi, wo, trans_dir);
        }
        LM_UNREACHABLE_RETURN();
    }

    virtual Vec3 eval_contrb_endpoint(const SceneInteraction& sp) const override {
        if (!sp.is_type(SceneInteraction::Endpoint)) {
            LM_THROW_EXCEPTION(Error::Unsupported,
                "eval_contrb_endpoint() function only supports endpoint interactions.");
        }
        // Always 1 for now
        return Vec3(1_f);
    }

    virtual std::optional<Vec3> reflectance(const SceneInteraction& sp) const override {
        if (!sp.is_type(SceneInteraction::SurfaceInteraction)) {
            LM_THROW_EXCEPTION(Error::Unsupported,
                "reflectance() function only supports surface interactions.");
        }
        const auto& primitive = nodes_.at(sp.primitive).primitive;
        return primitive.material->reflectance(sp.geom);
    }
};

LM_COMP_REG_IMPL(Scene_, "scene::default");

LM_NAMESPACE_END(LM_NAMESPACE)
