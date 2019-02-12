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

class Scene_ final : public Scene {
private:
    std::vector<Primitive> primitives_;
    Ptr<Accel> accel_;
    int camera_;                // Camera primitive index
    std::vector<int> lights_;   // Light primitive indices
    int envLight_ = -1;         // Environment light primitive index

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(primitives_, accel_, camera_, lights_);
    }

    virtual void foreachUnderlying(const ComponentVisitor& visit) override {
        comp::visit(visit, accel_);
        for (auto& primitive : primitives_) {
            comp::visit(visit, primitive.mesh);
            comp::visit(visit, primitive.material);
            comp::visit(visit, primitive.light);
            comp::visit(visit, primitive.camera);
        }
    }

public:
    virtual bool loadPrimitive(const Component& assetGroup, Mat4 transform, const Json& prop) override {
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
            return assetGroup.underlying(it.value().get<std::string>().c_str());
        };

        // Add a primitive entry
        primitives_.push_back(Primitive{
            int(primitives_.size()),
            Transform(transform),
            dynamic_cast<Mesh*>(getAssetRefBy("mesh")),
            dynamic_cast<Material*>(getAssetRefBy("material")),
            dynamic_cast<Light*>(getAssetRefBy("light")),
            dynamic_cast<Camera*>(getAssetRefBy("camera"))
        });

        const auto& primitive = primitives_.back();
        if (primitive.camera && primitive.light) {
            LM_ERROR("Primitive cannot be both camera and light");
            return false;
        }
        if (primitive.camera) {
            camera_ = primitives_.back().index;
        }
        if (primitive.light) {
            const int lightIndex = primitive.index;
            lights_.push_back(lightIndex);
            if (primitive.light->isInfinite()) {
                // Environment light
                envLight_ = lightIndex;
            }
        }

        return true;
    }

    virtual bool loadPrimitives(const Component& assetGroup, Mat4 transform, const std::string& modelName) override {
        auto* model = assetGroup.underlying<Model>(modelName);
        if (!model) {
            return false;
        }
        model->createPrimitives([&](Component* mesh, Component* material, Component* light) {
            primitives_.push_back(Primitive{
                int(primitives_.size()),
                Transform(transform),
                dynamic_cast<Mesh*>(mesh),
                dynamic_cast<Material*>(material),
                dynamic_cast<Light*>(light),
                nullptr
            });
            if (primitives_.back().light) {
                lights_.push_back(primitives_.back().index);
            }
        });
        return true;
    }

    virtual void foreachTriangle(const ProcessTriangleFunc& processTriangle) const override {
        for (const auto& primitive : primitives_) {
            if (!primitive.mesh) {
                continue;
            }
            primitive.mesh->foreachTriangle([&](int face, Mesh::Point p1, Mesh::Point p2, Mesh::Point p3) {
                processTriangle(
                    primitive.index,
                    face,
                    primitive.transform.M * Vec4(p1.p, 1_f),
                    primitive.transform.M * Vec4(p2.p, 1_f),
                    primitive.transform.M * Vec4(p3.p, 1_f)
                );
            });
        }
    }

    virtual void foreachPrimitive(const ProcessPrimitiveFunc& processPrimitive) const override {
        for (const auto& primitive : primitives_) {
            processPrimitive(primitive);
        }
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
            if (envLight_ < 0) {
                return {};
            }
            return SurfacePoint{
                envLight_,
                0,
                PointGeometry::makeInfinite(-ray.d),
                true
            };
        }
        const auto [t, uv, primitiveIndex, face] = *hit;
        const auto& primitive = primitives_.at(primitiveIndex);
        const auto p = primitive.mesh->surfacePoint(face, uv);
        return SurfacePoint{
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
        return primitives_.at(sp.primitive).light;
    }

    virtual bool isSpecular(const SurfacePoint& sp) const override {
        const auto& primitive = primitives_.at(sp.primitive);
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

    virtual Ray primaryRay(Vec2 rp) const {
        return primitives_.at(camera_).camera->primaryRay(rp);
    }

    virtual std::optional<RaySample> sampleRay(Rng& rng, const SurfacePoint& sp, Vec3 wi) const override {
        const auto* material = primitives_.at(sp.primitive).material;
        if (!material) {
            return {};
        }
        const auto s = material->sample(rng, sp.geom, wi);
        if (!s) {
            return {};
        }
        return RaySample{
            SurfacePoint{
                sp.primitive,
                s->comp,
                sp.geom,
                false
            },
            s->wo,
            s->weight
        };
    }

    virtual std::optional<RaySample> samplePrimaryRay(Rng& rng, Vec4 window) const override {
        const auto s = primitives_.at(camera_).camera->samplePrimaryRay(rng, window);
        if (!s) {
            return {};
        }
        return RaySample{
            SurfacePoint{
                camera_,
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
        const auto& primitive = primitives_.at(lights_[i]);
        const auto s = primitive.light->sample(rng, sp.geom, primitive.transform);
        if (!s) {
            return {};
        }
        return RaySample{
            SurfacePoint{
                lights_[i],
                0,
                s->geom,
                true
            },
            s->wo,
            s->weight / pL
        };
    }
    
    virtual Float pdf(const SurfacePoint& sp, Vec3 wi, Vec3 wo) const override {
        const auto& prim = primitives_.at(sp.primitive);
        return prim.material->pdf(sp.geom, sp.comp, wi, wo);
    }

    virtual Float pdfLight(const SurfacePoint& sp, const SurfacePoint& spL, Vec3 wo) const override {
        const auto& prim = primitives_.at(spL.primitive);
        const auto pL = 1_f / int(lights_.size());
        return prim.light->pdf(sp.geom, spL.geom, prim.transform, wo) * pL;
    }

    // ------------------------------------------------------------------------

    virtual Vec3 evalBsdf(const SurfacePoint& sp, Vec3 wi, Vec3 wo) const override {
        const auto& prim = primitives_.at(sp.primitive);
        if (sp.endpoint) {
            if (sp.primitive == camera_) {
                return prim.camera->eval(sp.geom, wo);
            }
            else {
                return prim.light->eval(sp.geom, wo);
            }
        }
        return prim.material->eval(sp.geom, sp.comp, wi, wo);
    }

    virtual Vec3 evalContrbEndpoint(const SurfacePoint& sp, Vec3 wo) const override {
        const auto& prim = primitives_.at(sp.primitive);
        if (!prim.light) {
            return {};
        }
        return prim.light->eval(sp.geom, wo);
    }

    virtual std::optional<Vec3> reflectance(const SurfacePoint& sp) const override {
        const auto& prim = primitives_.at(sp.primitive);
        if (!prim.material) {
            return {};
        }
        return prim.material->reflectance(sp.geom, sp.comp);
    }
};

LM_COMP_REG_IMPL(Scene_, "scene::default");

LM_NAMESPACE_END(LM_NAMESPACE)
