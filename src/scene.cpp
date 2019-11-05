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
    std::unordered_map<std::string, int> assetIndexMap_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(assetIndexMap_, assets_);
    }

    virtual void foreachUnderlying(const ComponentVisitor& visitor) override {
        for (auto& asset : assets_) {
            comp::visit(visitor, asset);
        }
    }

    virtual Component* underlying(const std::string& name) const override {
        auto it = assetIndexMap_.find(name);
        if (it == assetIndexMap_.end()) {
            LM_ERROR("Invalid asset name [name='{}']", name);
            return nullptr;
        }
        return assets_.at(it->second).get();
    }

private:
    bool validAssetName(const std::string& name) const {
        std::regex regex(R"x([:\w_-]+)x");
        std::smatch match;
        return std::regex_match(name, match, regex);
    }

public:
    std::optional<std::string> loadAsset(const std::string& name, const std::string& implKey, const Json& prop) {
        LM_INFO("Loading asset [name='{}']", name);
        LM_INDENT();

        // Check if asset name is valid
        if (!validAssetName(name)) {
            LM_ERROR("Invalid asset name [name='{}']", name);
            return {};
        }

        // Check if the asset with given name has been already loaded
        const auto it = assetIndexMap_.find(name);
        const bool found = it != assetIndexMap_.end();
        if (found) {
            LM_INFO("Asset [name='{}'] has been already loaded. Replacing..", name);
        }

        // Create an instance of the asset
        auto p = comp::createWithoutConstruct<Component>(implKey, makeLoc(loc(), name));
        if (!p) {
            LM_ERROR("Failed to create an asset [name='{}', key='{}']", name, implKey);
            return {};
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
                    comp->foreachUnderlying(visitor);
                }
                else {
                    comp::updateWeakRef(comp);
                }
            };
            comp::get<lm::Component>("$")->foreachUnderlying(visitor);
        }
        else {
            // Register as a new asset
            assetIndexMap_[name] = int(assets_.size());
            assets_.push_back(std::move(p));
            asset = assets_.back().get();

            // Initialize the asset
            asset->construct(prop);
        }

        return asset->loc();
    }
};

LM_COMP_REG_IMPL(Assets, "assets::default");

// ------------------------------------------------------------------------------------------------

struct LightPrimitiveIndex {
    Transform globalTransform; // Global transform matrix
    int index;                 // Primitive node index

    template <typename Archive>
    void serialize(Archive& ar) {
        ar(globalTransform, index);
    }
};

class Scene_ final : public Scene {
private:
    Ptr<Assets> assets_;                            // Underlying assets
    std::vector<SceneNode> nodes_;                  // Scene nodes
    Ptr<Accel> accel_;                              // Acceleration structure
    std::optional<int> camera_;                     // Camera index
    std::vector<LightPrimitiveIndex> lights_;       // Primitive node indices of lights and global transforms
    std::unordered_map<int, int> lightIndicesMap_;  // Map from node indices to light indices.
    std::optional<int> envLight_;                   // Environment light index
    std::optional<int> medium_;                     // Medium index

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(assets_, nodes_, accel_, camera_, lights_, lightIndicesMap_, envLight_);
    }

    virtual void foreachUnderlying(const ComponentVisitor& visit) override {
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
        assets_ = comp::create<Assets>("assets::default", makeLoc("assets"));
        // Index 0 is fixed to the scene group
        nodes_.push_back(SceneNode::makeGroup(0, false, {}));
    }

public:
    virtual bool renderable() const override {
        if (nodes_.size() == 1) {
            LM_ERROR("Missing primitives. Use lm::primitive() function to add primitives.");
            return false;
        }
        if (!camera_) {
            LM_ERROR("Missing camera primitive. Use lm::primitive() function to add camera primitive.");
            return false;
        }
        if (!accel_) {
            LM_ERROR("Missing acceleration structure. Use lm::build() function before rendering.");
            return false;
        }
        return true;
    }

    // --------------------------------------------------------------------------------------------

    virtual std::optional<std::string> loadAsset(const std::string& name, const std::string& implKey, const Json& prop) override {
        return assets_->loadAsset(name, implKey, prop);
    }

    // --------------------------------------------------------------------------------------------

    virtual int rootNode() override {
        return 0;
    }

    virtual int createNode(SceneNodeType type, const Json& prop) override {
        if (type == SceneNodeType::Primitive) {
            // Find an asset by property name
            const auto getAssetRefBy = [&](const std::string& propName) -> Component* {
                const auto it = prop.find(propName);
                if (it == prop.end()) {
                    return nullptr;
                }
                return comp::get<Component>(it.value().get<std::string>());
            };

            // Node index
            const int index = int(nodes_.size());

            // Get asset references
            auto* mesh = dynamic_cast<Mesh*>(getAssetRefBy("mesh"));
            auto* material = dynamic_cast<Material*>(getAssetRefBy("material"));
            auto* light = dynamic_cast<Light*>(getAssetRefBy("light"));
            auto* camera = dynamic_cast<Camera*>(getAssetRefBy("camera"));
            auto* medium = dynamic_cast<Medium*>(getAssetRefBy("medium"));

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
            if (light && light->isInfinite()) {
                if (envLight_) {
                    LM_ERROR("Environment light is already registered. "
                             "You can register only one environment light in the scene.");
                    return false;
                }
                envLight_ = index;
            }

            // Medium
            if (medium) {
                // For now, consider the medium as global asset.
                medium_ = index;
            }

            // Create primitive node
            nodes_.push_back(SceneNode::makePrimitive(index, mesh, material, light, camera, medium));

            return index;
        }
            
        // ----------------------------------------------------------------------------------------
        
        if (type == SceneNodeType::Group) {
            const int index = int(nodes_.size());
            nodes_.push_back(SceneNode::makeGroup(
                index,
                json::value<bool>(prop, "instanced", false),
                json::valueOrNone<Mat4>(prop, "transform")
            ));
            return index;
        }

        // ----------------------------------------------------------------------------------------

        LM_UNREACHABLE_RETURN();
    }

    virtual void addChild(int parent, int child) override {
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

    virtual void addChildFromModel(int parent, const std::string& modelLoc) override {
        if (parent < 0 || parent >= int(nodes_.size())) {
            LM_ERROR("Missing parent index [index='{}'", parent);
            return;
        }

        auto* model = comp::get<Model>(modelLoc);
        if (!model) {
            return;
        }
        
        model->createPrimitives([&](Component* mesh, Component* material, Component* light) {
            const int index = int(nodes_.size());
            nodes_.push_back(SceneNode::makePrimitive(
                index,
                dynamic_cast<Mesh*>(mesh),
                dynamic_cast<Material*>(material),
                dynamic_cast<Light*>(light),
                nullptr,
                nullptr));
            addChild(parent, index);
        });
    }

    virtual int createGroupFromModel(const std::string& modelLoc) override {
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
        model->foreachNode([&](const SceneNode& modelNode) {
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

    virtual void traversePrimitiveNodes(const NodeTraverseFunc& traverseFunc) const override {
        std::function<void(int, Mat4)> visit = [&](int index, Mat4 globalTransform) {
            const auto& node = nodes_.at(index);
            traverseFunc(node, globalTransform);
            if (node.type == SceneNodeType::Group) {
                const auto M = node.group.localTransform
                    ? globalTransform * *node.group.localTransform
                    : globalTransform;
                for (int child : node.group.children) {
                    visit(child, M);
                }
            }
        };
        visit(0, Mat4(1_f));
    }

    virtual void visitNode(int nodeIndex, const VisitNodeFunc& visit) const override {
        visit(nodes_.at(nodeIndex));
    }

    virtual const SceneNode& nodeAt(int nodeIndex) const override {
        return nodes_.at(nodeIndex);
    }

    // --------------------------------------------------------------------------------------------

    virtual void build(const std::string& name, const Json& prop) override {
        // Update light indices
        // We keep the global transformation of the light primitive as well as the references.
        // We need to recompute the indices when an update of the scene happens,
        // because the global tranformation can only be obtained by traversing the nodes.
        lightIndicesMap_.clear();
        lights_.clear();
        traversePrimitiveNodes([&](const SceneNode& node, Mat4 globalTransform) {
            if (node.type == SceneNodeType::Primitive && node.primitive.light) {
                lightIndicesMap_[node.index] = int(lights_.size());
                lights_.push_back({ Transform(globalTransform), node.index });
            }
        });

        // Build acceleration structure
        accel_ = comp::create<Accel>(name, makeLoc(loc(), "accel"), prop);
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
            if (!envLight_) {
                return {};
            }
            return SceneInteraction::makeLightEndpoint(
                *envLight_,
                0,
                PointGeometry::makeInfinite(-ray.d));
        }
        const auto [t, uv, globalTransform, primitiveIndex, faceIndex] = *hit;
        const auto& primitive = nodes_.at(primitiveIndex).primitive;
        const auto p = primitive.mesh->surfacePoint(faceIndex, uv);
        return SceneInteraction::makeSurfaceInteraction(
            primitiveIndex,
            -1,
            PointGeometry::makeOnSurface(
                globalTransform.M * Vec4(p.p, 1_f),
                glm::normalize(globalTransform.normalM * p.n),
                p.t
            )
        );
    }

    // --------------------------------------------------------------------------------------------

    virtual bool isLight(const SceneInteraction& sp) const override {
        const auto& primitive = nodes_.at(sp.primitive).primitive;
        return sp.medium
            ? primitive.medium->isEmitter()
            : primitive.light != nullptr;
    }

    virtual bool isSpecular(const SceneInteraction& sp) const override {
        const auto& primitive = nodes_.at(sp.primitive).primitive;
        if (sp.medium) {
            return primitive.medium->phase()->isSpecular(sp.geom);
        }
        if (sp.endpoint) {
            if (primitive.light) {
                return primitive.light->isSpecular(sp.geom, sp.comp);
            }
            else if (primitive.camera) {
                return primitive.camera->isSpecular(sp.geom);
            }
            LM_UNREACHABLE_RETURN();
        }
        return primitive.material->isSpecular(sp.geom, sp.comp);
    }

    // --------------------------------------------------------------------------------------------

    virtual Ray primaryRay(Vec2 rp, Float aspectRatio) const {
        return nodes_.at(*camera_).primitive.camera->primaryRay(rp, aspectRatio);
    }

    virtual std::optional<RaySample> sampleRay(Rng& rng, const SceneInteraction& sp, Vec3 wi) const override {
        if (sp.medium) {
            // Medium interaction
            const auto& primitive = nodes_.at(sp.primitive).primitive;
            const auto s = primitive.medium->phase()->sample(rng, sp.geom, wi);
            if (!s) {
                return {};
            }
            return RaySample{
                sp,
                s->wo,
                s->weight
            };
        }
        else if (sp.terminator && sp.terminator == TerminatorType::Camera) {
            // Endpoint
            const auto* camera = nodes_.at(*camera_).primitive.camera;
            const auto s = camera->samplePrimaryRay(rng, sp.cameraCond.window, sp.cameraCond.aspectRatio);
            if (!s) {
                return {};
            }
            return RaySample{
                SceneInteraction::makeCameraEndpoint(
                    *camera_,
                    0,
                    s->geom,
                    sp.cameraCond.window,
                    sp.cameraCond.aspectRatio
                ),
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
                SceneInteraction::makeSurfaceInteraction(
                    sp.primitive,
                    s->comp,
                    sp.geom
                ),
                s->wo,
                s->weight
            };
        }
    }

    virtual Float pdfComp(const SceneInteraction& sp, Vec3 wi) const override {
        const auto& primitive = nodes_.at(sp.primitive).primitive;
        if (sp.medium) {
            return 1_f;
        }
        else {
            if (!primitive.material) {
                return 1_f;
            }
            return primitive.material->pdfComp(sp.geom, sp.comp, wi);
        }
    }

    virtual std::optional<Vec2> rasterPosition(Vec3 wo, Float aspectRatio) const override {
        const auto* camera = nodes_.at(*camera_).primitive.camera;
        return camera->rasterPosition(wo, aspectRatio);
    }

    virtual std::optional<RaySample> sampleDirectLight(Rng& rng, const SceneInteraction& sp) const override {
        // Sample a light
        const int n  = int(lights_.size());
        const int i  = glm::clamp(int(rng.u() * n), 0, n-1);
        const auto pL = 1_f / n;
        
        // Sample a position on the light
        const auto light = lights_.at(i);
        const auto& primitive = nodes_.at(light.index).primitive;
        const auto s = primitive.light->sample(rng, sp.geom, light.globalTransform);
        if (!s) {
            return {};
        }
        return RaySample{
            SceneInteraction::makeLightEndpoint(
                light.index,
                s->comp,
                s->geom
            ),
            s->wo,
            s->weight / pL
        };
    }
    
    virtual Float pdf(const SceneInteraction& sp, Vec3 wi, Vec3 wo) const override {
        const auto& primitive = nodes_.at(sp.primitive).primitive;
        if (sp.medium) {
            return primitive.medium->phase()->pdf(sp.geom, wi, wo);
        }
        else if (sp.endpoint) {
            if (primitive.light) {
                LM_THROW_EXCEPTION_DEFAULT(Error::Unimplemented);
            }
            else if (primitive.camera) {
                return primitive.camera->pdf(wo, sp.cameraCond.aspectRatio);
            }
            LM_UNREACHABLE_RETURN();
        }
        else {
            return primitive.material->pdf(sp.geom, sp.comp, wi, wo);
        }
    }

    virtual Float pdfDirectLight(const SceneInteraction& sp, const SceneInteraction& spL, Vec3 wo) const override {
        const auto& primitive = nodes_.at(spL.primitive).primitive;
        const auto lightTransform = lights_.at(lightIndicesMap_.at(spL.primitive)).globalTransform;
        const auto pL = 1_f / int(lights_.size());
        return primitive.light->pdf(sp.geom, spL.geom, spL.comp, lightTransform, wo) * pL;
    }

    // --------------------------------------------------------------------------------------------

    virtual std::optional<DistanceSample> sampleDistance(Rng& rng, const SceneInteraction& sp, Vec3 wo) const override {
        // Intersection to next surface
        const auto hit = intersect({ sp.geom.p, wo }, Eps, Inf);
        const auto dist = hit && !hit->geom.infinite ? glm::length(hit->geom.p - sp.geom.p) : Inf;

        // Sample a distance
        const auto* medium = nodes_.at(*medium_).primitive.medium;
        const auto ds = medium->sampleDistance(rng, { sp.geom.p, wo }, 0_f, dist);
        if (ds && ds->medium) {
            // Medium interaction
            return DistanceSample{
                SceneInteraction::makeMediumInteraction(
                    *medium_,
                    0,
                    PointGeometry::makeDegenerated(ds->p)
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

    virtual Vec3 evalTransmittance(Rng& rng, const SceneInteraction& sp1, const SceneInteraction& sp2) const override {
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
        return medium->evalTransmittance(rng, { sp1.geom.p, wo }, 0_f, dist);
    }

    // --------------------------------------------------------------------------------------------

    virtual Vec3 evalContrb(const SceneInteraction& sp, Vec3 wi, Vec3 wo) const override {
        const auto& primitive = nodes_.at(sp.primitive).primitive;
        if (sp.medium) {
            // Medium interaction
            return primitive.medium->phase()->eval(sp.geom, wi, wo);
        }
        else {
            // Surface interaction
            if (sp.endpoint) {
                if (primitive.camera) {
                    return primitive.camera->eval(wo, sp.cameraCond.aspectRatio);
                }
                else if (primitive.light) {
                    return primitive.light->eval(sp.geom, sp.comp, wo);
                }
                LM_UNREACHABLE();
            }
            return primitive.material->eval(sp.geom, sp.comp, wi, wo);
        }
    }

    virtual Vec3 evalContrbEndpoint(const SceneInteraction& sp, Vec3 wo) const override {
        const auto& primitive = nodes_.at(sp.primitive).primitive;
        if (!primitive.light) {
            return {};
        }
        return primitive.light->eval(sp.geom, sp.comp, wo);
    }

    virtual std::optional<Vec3> reflectance(const SceneInteraction& sp) const override {
        const auto& primitive = nodes_.at(sp.primitive).primitive;
        if (!primitive.material) {
            return {};
        }
        return primitive.material->reflectance(sp.geom, sp.comp);
    }
};

LM_COMP_REG_IMPL(Scene_, "scene::default");

LM_NAMESPACE_END(LM_NAMESPACE)
