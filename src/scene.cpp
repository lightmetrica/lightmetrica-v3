/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/scene.h>
#include <lm/assets.h>
#include <lm/accel.h>
#include <lm/mesh.h>
#include <lm/camera.h>
#include <lm/material.h>
#include <lm/light.h>
#include <lm/model.h>
#include <lm/logger.h>
#include <lm/serial.h>
#include <lm/json.h>
#include <lm/medium.h>
#include <lm/phase.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

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
    std::vector<SceneNode> nodes_;                  // Scene nodes
    Ptr<Accel> accel_;                              // Acceleration structure
    std::optional<int> camera_;                     // Camera index
    std::vector<LightPrimitiveIndex> lights_;       // Primitive node indices of lights and global transforms
    std::unordered_map<int, int> lightIndicesMap_;  // Map from node indices to light indices.
    std::optional<int> envLight_;                   // Environment light index
    std::optional<int> medium_;                     // Medium index

public:
    Scene_() {
        // Index 0 is fixed to the scene group
        nodes_.push_back(SceneNode::makeGroup(0, false, {}));
    }

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(nodes_, accel_, camera_, lights_, lightIndicesMap_, envLight_);
    }

    virtual void foreachUnderlying(const ComponentVisitor& visit) override {
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
        if (name == "accel") {
            return accel_.get();
        }
        if (name == "camera") {
            return nodes_.at(*camera_).primitive.camera;
        }
        return nullptr;
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

    // ------------------------------------------------------------------------

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
            
        // ----------------------------------------------------------------
        
        if (type == SceneNodeType::Group) {
            const int index = int(nodes_.size());
            nodes_.push_back(SceneNode::makeGroup(
                index,
                json::value<bool>(prop, "instanced", false),
                json::valueOrNone<Mat4>(prop, "transform")
            ));
            return index;
        }

        // ----------------------------------------------------------------

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

    // ------------------------------------------------------------------------

    virtual void traverseNodes(const NodeTraverseFunc& traverseFunc) const override {
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

    // ------------------------------------------------------------------------

    virtual void build(const std::string& name, const Json& prop) override {
        // Update light indices
        // We keep the global transformation of the light primitive as well as the references.
        // We need to recompute the indices when an update of the scene happens,
        // because the global tranformation can only be obtained by traversing the nodes.
        lightIndicesMap_.clear();
        lights_.clear();
        if (envLight_) {
            lightIndicesMap_[*envLight_] = 0;
            lights_.push_back({ Transform(Mat4(1_f)), *envLight_ });
        }
        traverseNodes([&](const SceneNode& node, Mat4 globalTransform) {
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
            return SceneInteraction{
                *envLight_,
                0,
                PointGeometry::makeInfinite(-ray.d),
                true,
                false
            };
        }
        const auto [t, uv, globalTransform, primitiveIndex, faceIndex] = *hit;
        const auto& primitive = nodes_.at(primitiveIndex).primitive;
        const auto p = primitive.mesh->surfacePoint(faceIndex, uv);
        return SceneInteraction{
            primitiveIndex,
            -1,
            PointGeometry::makeOnSurface(
                globalTransform.M * Vec4(p.p, 1_f),
                globalTransform.normalM * p.n,
                p.t
            ),
            false,
            false
        };
    }

    // ------------------------------------------------------------------------

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
                return primitive.light->isSpecular(sp.geom);
            }
            else if (primitive.camera) {
                return primitive.camera->isSpecular(sp.geom);
            }
            LM_UNREACHABLE_RETURN();
        }
        return primitive.material->isSpecular(sp.geom, sp.comp);
    }

    // ------------------------------------------------------------------------

    virtual Ray primaryRay(Vec2 rp, Float aspectRatio) const {
        return nodes_.at(*camera_).primitive.camera->primaryRay(rp, aspectRatio);
    }

    virtual std::optional<RaySample> sampleRay(Rng& rng, const SceneInteraction& sp, Vec3 wi) const override {
        const auto& primitive = nodes_.at(sp.primitive).primitive;
        if (sp.medium) {
            // Medium interaction
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
        else {
            // Surface interaction
            if (!primitive.material) {
                return {};
            }
            const auto s = primitive.material->sample(rng, sp.geom, wi);
            if (!s) {
                return {};
            }
            return RaySample{
                SceneInteraction{
                    sp.primitive,
                    s->comp,
                    sp.geom,
                    false,
                    false
                },
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

    virtual std::optional<RaySample> samplePrimaryRay(Rng& rng, Vec4 window, Float aspectRatio) const override {
        const auto s = nodes_.at(*camera_).primitive.camera->samplePrimaryRay(rng, window, aspectRatio);
        if (!s) {
            return {};
        }
        return RaySample{
            SceneInteraction{
                *camera_,
                0,
                s->geom,
                true,
                false
            },
            s->wo,
            s->weight
        };
    }

    virtual std::optional<RaySample> sampleLight(Rng& rng, const SceneInteraction& sp) const override {
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
            SceneInteraction{
                light.index,
                0,
                s->geom,
                true,
                false
            },
            s->wo,
            s->weight / pL
        };
    }
    
    virtual Float pdf(const SceneInteraction& sp, Vec3 wi, Vec3 wo) const override {
        const auto& primitive = nodes_.at(sp.primitive).primitive;
        if (sp.medium) {
            return primitive.medium->phase()->pdf(sp.geom, wi, wo);
        }
        else {
            return primitive.material->pdf(sp.geom, sp.comp, wi, wo);
        }
    }

    virtual Float pdfLight(const SceneInteraction& sp, const SceneInteraction& spL, Vec3 wo) const override {
        const auto& primitive = nodes_.at(spL.primitive).primitive;
        const auto lightTransform = lights_.at(lightIndicesMap_.at(spL.primitive)).globalTransform;
        const auto pL = 1_f / int(lights_.size());
        return primitive.light->pdf(sp.geom, spL.geom, lightTransform, wo) * pL;
    }

    // ------------------------------------------------------------------------

    virtual std::optional<DistanceSample> sampleDistance(Rng& rng, const SceneInteraction& sp, Vec3 wo) const override {
        // Intersection to next surface
        const auto hit = intersect(Ray{ sp.geom.p, wo }, Eps, Inf);
        const auto dist = hit ? glm::length(hit->geom.p - sp.geom.p) : Inf;
        
        // Sample a distance
        const auto* medium = nodes_.at(*medium_).primitive.medium;
        const auto ds = medium->sampleDistance(rng, sp.geom, wo, dist);
        assert(ds);
        
        if (ds && ds->medium) {
            // Medium interaction
            return DistanceSample{
                SceneInteraction{
                    *medium_,
                    0,
                    PointGeometry::makeDegenerated(ds->p),
                    false,
                    true
                },
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

    virtual std::optional<Vec3> evalTransmittance(Rng& rng, const SceneInteraction& sp1, const SceneInteraction& sp2) const override {
        if (!visible(sp1, sp2)) {
            return {};
        }
        if (!medium_) {
            return Vec3(1_f);
        }
        return nodes_.at(*medium_).primitive.medium->evalTransmittance(rng, sp1.geom, sp2.geom);
    }

    // ------------------------------------------------------------------------

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
                    return primitive.camera->eval(sp.geom, wo);
                }
                else if (primitive.light) {
                    return primitive.light->eval(sp.geom, wo);
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
        return primitive.light->eval(sp.geom, wo);
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
