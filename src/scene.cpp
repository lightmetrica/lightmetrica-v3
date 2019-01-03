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

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

struct Primitive {
    int index;					// Primitive index
	Transform transform;		// Transform associated to the primitive
    const Mesh* mesh;			// Underlying assets
    const Material* material;
    const Light* light;
    const Camera* camera;
};

class Scene_ final : public Scene {
private:
	std::vector<Primitive> primitives_;
	Ptr<Accel> accel_;
	int camera_;			   // Camera primitive index
	std::vector<int> lights_;  // Light primitive indices

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

		// Underlying components of primitive
		const auto* mesh     = dynamic_cast<Mesh*>(getAssetRefBy("mesh"));
		const auto* material = dynamic_cast<Material*>(getAssetRefBy("material"));
		const auto* light    = dynamic_cast<Light*>(getAssetRefBy("light"));
		const auto* camera   = dynamic_cast<Camera*>(getAssetRefBy("camera"));

		// Primitive cannot be both camera and light
		if (camera && light) {
			LM_ERROR("Primitive cannot be both camera and light");
			return false;
		}

		// Add a primitive entry
        primitives_.push_back(Primitive{
            int(primitives_.size()),
            Transform(transform),
            mesh,
            material,
            light,
            camera
        });
        if (primitives_.back().camera) {
            camera_ = primitives_.back().index;
        }
		if (primitives_.back().light) {
			lights_.push_back(primitives_.back().index);
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
            primitive.mesh->foreachTriangle([&](int face, Vec3 p1, Vec3 p2, Vec3 p3) {
                processTriangle(
                    primitive.index,
                    face,
					primitive.transform.M * Vec4(p1, 1_f),
					primitive.transform.M * Vec4(p2, 1_f),
					primitive.transform.M * Vec4(p3, 1_f)
                );
            });
        }
    }

    // ------------------------------------------------------------------------

    virtual void build(const std::string& name) override {
        accel_ = comp::create<Accel>(name, this);
        if (!accel_) {
            return;
        }
        accel_->build(*this);
    }

    virtual std::optional<SurfacePoint> intersect(Ray ray, Float tmin, Float tmax) const override {
        const auto hit = accel_->intersect(ray, tmin, tmax);
        if (!hit) {
            return {};
        }
        const auto [t, uv, primitiveIndex, face] = *hit;
		const auto& primitive = primitives_.at(primitiveIndex);
        const auto p = primitive.mesh->surfacePoint(face, uv);
		return SurfacePoint(
			primitiveIndex,
			-1,
			primitive.transform.M * Vec4(p.p, 1_f),
			primitive.transform.normalM * p.n,
			p.t);
    }

    // ------------------------------------------------------------------------

    virtual bool isLight(const SurfacePoint& sp) const override {
        return primitives_.at(sp.primitive).light;
    }

    virtual bool isSpecular(const SurfacePoint& sp) const override {
        const auto& primitive = primitives_.at(sp.primitive);
        if (sp.endpoint) {
            if (primitive.light) {
                return primitive.light->isSpecular(sp);
            }
            else if (primitive.camera) {
                return primitive.camera->isSpecular(sp);
            }
            LM_UNREACHABLE_RETURN();
        }
        return primitive.material->isSpecular(sp);
    }

    // ------------------------------------------------------------------------

    virtual Ray primaryRay(Vec2 rp) const {
        return primitives_.at(camera_).camera->primaryRay(rp);
    }

    virtual std::optional<RaySample> sampleRay(Rng& rng, const SurfacePoint& sp, Vec3 wi) const override {
        return primitives_.at(sp.primitive).material->sampleRay(rng, sp, wi);
    }

    virtual std::optional<RaySample> samplePrimaryRay(Rng& rng, Vec4 window) const override {
        auto rs = primitives_.at(camera_).camera->samplePrimaryRay(rng, window);
        if (!rs) {
            return {};
        }
        return rs->asPrimitive(camera_).asEndpoint(true);
    }

	virtual std::optional<LightSample> sampleLight(Rng& rng, const SurfacePoint& sp) const override {
		// Sample a light
		const int n  = int(lights_.size());
		const int i  = glm::clamp(int(rng.u() * n), 0, n-1);
		const auto pL = 1_f / n;
		
		// Sample a position on the light
		const auto& primitive = primitives_.at(lights_[i]);
		return primitive.light->sampleLight(rng, sp, primitive.transform);
	}
	
	// ------------------------------------------------------------------------

	virtual Vec3 evalBsdf(const SurfacePoint& sp, Vec3 wi, Vec3 wo) const override {
		if (sp.endpoint) {
			if (sp.primitive == camera_) {
				return primitives_.at(sp.primitive).camera->eval(sp, wo);
			}
			else {
				return primitives_.at(sp.primitive).light->eval(sp, wo);
			}
		}
		return primitives_.at(sp.primitive).material->eval(sp, wi, wo);
	}

    virtual Vec3 evalContrbEndpoint(const SurfacePoint& sp, Vec3 wo) const override {
        const auto& primitive = primitives_.at(sp.primitive);
        if (!primitive.light) {
            return {};
        }
        return primitive.light->eval(sp, wo);
    }

    virtual std::optional<Vec3> reflectance(const SurfacePoint& sp) const override {
        const auto& primitive = primitives_.at(sp.primitive);
        if (!primitive.material) {
            return {};
        }
        return primitive.material->reflectance(sp);
    }
};

LM_COMP_REG_IMPL(Scene_, "scene::default");

LM_NAMESPACE_END(LM_NAMESPACE)
