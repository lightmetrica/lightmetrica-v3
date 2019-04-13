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

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// Primitive group representing a set of primitives used for instacing.
struct PrimitiveGroup {
    int index;                          // Group index.
    std::vector<Primitive> primitives;  // Underlying primitives.
    template <typename Archive>
    void serialize(Archive& ar) {
        ar(index, primitives);
    }
};

// Index describing a primitive in the scene.
struct PrimitiveIndex {
    int group;
    int primitive;
    template <typename Archive>
    void serialize(Archive& ar) {
        ar(group, primitive);
    }
};

class Scene_ final : public Scene {
private:
    std::vector<PrimitiveGroup> primitiveGroups_;
    std::unordered_map<std::string, int> primitiveGroupMap_;
    Ptr<Accel> accel_;
    std::optional<PrimitiveIndex> camera_;      // Camera primitive index
    std::vector<PrimitiveIndex> lights_;        // Light primitive indices
    std::optional<PrimitiveIndex> envLight_;    // Environment light primitive index

public:
    Scene_()
        : primitiveGroups_(1, PrimitiveGroup{}) // Root group
    {}

private:
    const Primitive& primitiveAt(PrimitiveIndex idx) const {
        return primitiveGroups_.at(idx.group).primitives.at(idx.primitive);
    }

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(primitiveGroups_, primitiveGroupMap_, accel_, camera_, lights_, envLight_);
    }

    virtual void foreachUnderlying(const ComponentVisitor& visit) override {
        comp::visit(visit, accel_);
        for (auto& primitiveGroup : primitiveGroups_) {
            for (auto& primitive : primitiveGroup.primitives) {
                comp::visit(visit, primitive.mesh);
                comp::visit(visit, primitive.material);
                comp::visit(visit, primitive.light);
                comp::visit(visit, primitive.camera);
            }
        }
    }

    virtual Component* underlying(const std::string& name) const override {
        if (name == "accel") {
            return accel_.get();
        }
        return nullptr;
    }

public:
    virtual bool renderable() const override {
        if (primitiveGroups_.front().primitives.empty()) {
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

    virtual bool loadPrimitive(int group, Mat4 transform, const Json& prop) override {
        // Select primitives
        if (group < 0 || group >= int(primitiveGroups_.size())) {
            LM_ERROR("Invalid group index [group='{}'", group);
            return false;
        }
        auto& primitives = primitiveGroups_.at(group).primitives;

        // --------------------------------------------------------------------

        // Helper function to find an asset by property name
        const auto getAssetRefBy = [&](const std::string& propName) -> Component* {
            if (propName.empty()) {
                return nullptr;
            }
            // Find reference to an asset
            const auto it = prop.find(propName);
            if (it == prop.end()) {
                return nullptr;
            }
            // Obtain the referenced asset
            return comp::get<Component>(it.value().get<std::string>());
        };

        // --------------------------------------------------------------------

        // Create primitive as primitive group
        if (prop.find("group") != prop.end()) {
            const int childGroup = prop["group"];
            primitives.push_back(Primitive::makePrimitiveGroup(
                group,
                int(primitives.size()),
                Transform(transform),
                childGroup));
        }
        // Create primitive from model
        else if (prop.find("model") != prop.end()) {
            const std::string modelLoc = prop["model"];
            auto* model = comp::get<Model>(modelLoc);
            if (!model) {
                return false;
            }
            model->createPrimitives([&](Component* mesh, Component* material, Component* light) {
                const auto primitive = Primitive::makeSceneObject(
                    group,
                    int(primitives.size()),
                    Transform(transform),
                    dynamic_cast<Mesh*>(mesh),
                    dynamic_cast<Material*>(material),
                    dynamic_cast<Light*>(light),
                    nullptr);
                if (primitive.light) {
                    lights_.push_back({ group, primitive.index });
                }
                primitives.push_back(primitive);
            });
        }
        // Create primitive from scene object
        else {
            const auto primitive = Primitive::makeSceneObject(
                group,
                int(primitives.size()),
                Transform(transform),
                dynamic_cast<Mesh*>(getAssetRefBy("mesh")),
                dynamic_cast<Material*>(getAssetRefBy("material")),
                dynamic_cast<Light*>(getAssetRefBy("light")),
                dynamic_cast<Camera*>(getAssetRefBy("camera")));

            if (primitive.camera && primitive.light) {
                LM_ERROR("Primitive cannot be both camera and light");
                return false;
            }
            if (primitive.camera) {
                if (group != 0) {
                    LM_ERROR("Camera must be registered to root group [group='{}']", group);
                    return false;
                }
                camera_ = PrimitiveIndex{ group, primitive.index };
            }
            if (primitive.light) {
                const auto lightIndex = PrimitiveIndex{ group, primitive.index };
                if (primitive.light->isInfinite()) {
                    // Environment light
                    if (group != 0) {
                        LM_ERROR("Environment light must be registered to root group [group='{}']", group);
                        return false;
                    }
                    envLight_ = lightIndex;
                }
                lights_.push_back(lightIndex);
            }

            // Add primitive
            primitives.push_back(primitive);
        }

        return true;
    }

    virtual int addGroup(const std::string& groupName) override {
        if (auto it = primitiveGroupMap_.find(groupName); it != primitiveGroupMap_.end()) {
            return it->second;
        }
        const int idx = int(primitiveGroups_.size());
        primitiveGroupMap_[groupName] = idx;
        primitiveGroups_.emplace_back();
        primitiveGroups_.back().index = idx;
        return idx;
    }

    virtual void foreachTriangle(const ProcessTriangleFunc& processTriangle) const override {
        foreachPrimitive([&](const Primitive& primitive, Mat4 transform) {
            const auto M = primitive.transform.M * transform;
            primitive.mesh->foreachTriangle([&](int face, const Mesh::Tri& tri) {
                processTriangle(
                    primitive.group,
                    primitive.index,
                    face,
                    M * Vec4(tri.p1.p, 1_f),
                    M * Vec4(tri.p2.p, 1_f),
                    M * Vec4(tri.p3.p, 1_f)
                );
            });
        });

#if 0
        // Traverse primitives
        // TODO: avoid cyclic graph
        std::function<void(int, Mat4)> visit = [&](int group, Mat4 transform) {
            const auto& primitiveGroup = primitiveGroups_.at(group);
            for (const auto& primitive : primitiveGroup.primitives) {
                if (primitive.childGroup != -1) {
                    visit(primitive.childGroup, primitive.transform.M * transform);
                }
                else {
                    const auto M = primitive.transform.M * transform;
                    primitive.mesh->foreachTriangle([&](int face, const Mesh::Tri& tri) {
                        processTriangle(
                            primitiveGroup.index,
                            primitive.index,
                            face,
                            M * Vec4(tri.p1.p, 1_f),
                            M * Vec4(tri.p2.p, 1_f),
                            M * Vec4(tri.p3.p, 1_f)
                        );
                    });
                }
            }
        };
        visit(0, Mat4(1_f));
#endif
#if 0
        for (const auto& primitiveGroup : primitiveGroups_) {
            for (const auto& primitive : primitiveGroup.primitives) {
                if (!primitive.mesh) {
                    continue;
                }
                primitive.mesh->foreachTriangle([&](int face, const Mesh::Tri& tri) {
                    processTriangle(
                        primitiveGroup.index,
                        primitive.index,
                        face,
                        primitive.transform.M * Vec4(tri.p1.p, 1_f),
                        primitive.transform.M * Vec4(tri.p2.p, 1_f),
                        primitive.transform.M * Vec4(tri.p3.p, 1_f)
                    );
                });
            }
        }
#endif
    }

    virtual void foreachPrimitive(const ProcessPrimitiveFunc& processPrimitive) const override {
        // Traverse primitives
        // TODO: avoid cyclic graph
        std::function<void(int, Mat4)> visit = [&](int group, Mat4 transform) {
            const auto& primitiveGroup = primitiveGroups_.at(group);
            for (const auto& primitive : primitiveGroup.primitives) {
                if (primitive.childGroup != -1) {
                    visit(primitive.childGroup, primitive.transform.M * transform);
                }
                else {
                    processPrimitive(primitive, transform);
                }
            }
        };
        visit(0, Mat4(1_f));
#if 0
        for (const auto& primitiveGroup : primitiveGroups_) {
            for (const auto& primitive : primitiveGroup.primitives) {
                processPrimitive(primitive);
            }
        }
#endif
    }

    // ------------------------------------------------------------------------

    virtual void build(const std::string& name, const Json& prop) override {
        accel_ = comp::create<Accel>(name, makeLoc(loc(), "accel"), prop);
        if (!accel_) {
            return;
        }
        accel_->build(*this);
    }

    virtual std::optional<SurfacePoint> intersect(Ray ray, Float tmin, Float tmax) const override {
        const auto hit = accel_->intersect(ray, tmin, tmax);
        if (!hit) {
            // Use environment light when tmax = Inf
            if (tmax < Inf) {
                return {};
            }
            if (!envLight_) {
                return {};
            }
            return SurfacePoint{
                envLight_->group,
                envLight_->primitive,
                0,
                PointGeometry::makeInfinite(-ray.d),
                true
            };
        }
        const auto [t, uv, groupIndex, primitiveIndex, faceIndex] = *hit;
        const auto& primitive = primitiveAt({ groupIndex, primitiveIndex });
        const auto p = primitive.mesh->surfacePoint(faceIndex, uv);
        return SurfacePoint{
            groupIndex,
            primitiveIndex,
            -1,
            PointGeometry::makeOnSurface(
                primitive.transform.M * Vec4(p.p, 1_f),
                primitive.transform.normalM * p.n,
                p.t
            ),
            false
        };
    }

    // ------------------------------------------------------------------------

    virtual bool isLight(const SurfacePoint& sp) const override {
        return primitiveGroups_.at(sp.group).primitives.at(sp.primitive).light;
    }

    virtual bool isSpecular(const SurfacePoint& sp) const override {
        const auto& primitive = primitiveAt({ sp.group, sp.primitive });
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
        return primitiveAt(*camera_).camera->primaryRay(rp, aspectRatio);
    }

    virtual std::optional<RaySample> sampleRay(Rng& rng, const SurfacePoint& sp, Vec3 wi) const override {
        const auto* material = primitiveAt({ sp.group, sp.primitive }).material;
        if (!material) {
            return {};
        }
        const auto s = material->sample(rng, sp.geom, wi);
        if (!s) {
            return {};
        }
        return RaySample{
            SurfacePoint{
                sp.group,
                sp.primitive,
                s->comp,
                sp.geom,
                false
            },
            s->wo,
            s->weight
        };
    }

    virtual std::optional<RaySample> samplePrimaryRay(Rng& rng, Vec4 window, Float aspectRatio) const override {
        const auto s = primitiveAt(*camera_).camera->samplePrimaryRay(rng, window, aspectRatio);
        if (!s) {
            return {};
        }
        return RaySample{
            SurfacePoint{
                camera_->group,
                camera_->primitive,
                0,
                s->geom,
                true
            },
            s->wo,
            s->weight
        };
    }

    virtual std::optional<RaySample> sampleLight(Rng& rng, const SurfacePoint& sp) const override {
        // Sample a light
        const int n  = int(lights_.size());
        const int i  = glm::clamp(int(rng.u() * n), 0, n-1);
        const auto pL = 1_f / n;
        
        // Sample a position on the light
        const auto& primitive = primitiveAt(lights_[i]);
        const auto s = primitive.light->sample(rng, sp.geom, primitive.transform);
        if (!s) {
            return {};
        }
        return RaySample{
            SurfacePoint{
                lights_[i].group,
                lights_[i].primitive,
                0,
                s->geom,
                true
            },
            s->wo,
            s->weight / pL
        };
    }
    
    virtual Float pdf(const SurfacePoint& sp, Vec3 wi, Vec3 wo) const override {
        const auto& prim = primitiveAt({ sp.group, sp.primitive });
        return prim.material->pdf(sp.geom, sp.comp, wi, wo);
    }

    virtual Float pdfLight(const SurfacePoint& sp, const SurfacePoint& spL, Vec3 wo) const override {
        const auto& prim = primitiveAt({ spL.group, spL.primitive });
        const auto pL = 1_f / int(lights_.size());
        return prim.light->pdf(sp.geom, spL.geom, prim.transform, wo) * pL;
    }

    // ------------------------------------------------------------------------

    virtual Vec3 evalBsdf(const SurfacePoint& sp, Vec3 wi, Vec3 wo) const override {
        const auto& prim = primitiveAt({ sp.group, sp.primitive });
        if (sp.endpoint) {
            if (prim.camera) {
                return prim.camera->eval(sp.geom, wo);
            }
            else if (prim.light) {
                return prim.light->eval(sp.geom, wo);
            }
            LM_UNREACHABLE();
        }
        return prim.material->eval(sp.geom, sp.comp, wi, wo);
    }

    virtual Vec3 evalContrbEndpoint(const SurfacePoint& sp, Vec3 wo) const override {
        const auto& prim = primitiveAt({ sp.group, sp.primitive });
        if (!prim.light) {
            return {};
        }
        return prim.light->eval(sp.geom, wo);
    }

    virtual std::optional<Vec3> reflectance(const SurfacePoint& sp) const override {
        const auto& prim = primitiveAt({ sp.group, sp.primitive });
        if (!prim.material) {
            return {};
        }
        return prim.material->reflectance(sp.geom, sp.comp);
    }
};

LM_COMP_REG_IMPL(Scene_, "scene::default");

LM_NAMESPACE_END(LM_NAMESPACE)
