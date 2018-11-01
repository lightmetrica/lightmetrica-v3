/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/scene.h>
#include <lm/assets.h>
#include <lm/accel.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// ----------------------------------------------------------------------------

class Primitive_ final : public Primitive {
private:
    // Function to find and obtain an asset by name
    template <typename T>
    const T* getAssetRefBy(const Json& prop, const std::string& propName) {
        if (propName.empty()) {
            return nullptr;
        }
        // Find reference to an asset
        const auto it = prop.find(propName);
        if (it == prop.end()) {
            return nullptr;
        }
        // Obtain the referenced asset
        return assets.underlying(it.value().get<std::string>().c_str());
    };

public:
    virtual bool construct(const Json& prop) {
        

        // 


        if (!prop.is_object()) {
            LM_ERROR("Invalid construction parameters");
            return false;
        }
        for (auto it = prop.begin(); it != prop.end(); ++it) {
            if (!it.value().is_string()) {
                LM_ERROR("Referenced component name must be string");
                return false;
            }
            // Find referenced asset
            const std::string name = it.value();
            auto* comp = context()->underlying(fmt::format("assets.{}", name));
            if (!comp) {
                return false;
            }
            compIndexMap_[it.key()] = comps_.size();
            comps_.push_back(comp);
        }
        return true;
    }

    virtual Mat4 transform() const override {
        return transform_;
    }

private:
    Mat4 transform_; 
    Mesh* mesh_;
    Material* material_;
    Light* light_;
    Sensor* sensor_;
};

LM_COMP_REG_IMPL(Primitive_, "primitive::default");

// ----------------------------------------------------------------------------

class Scene_ final : public Scene {
public:
    virtual Component* underlying(const std::string& name) const override {
        return primitives_[primitiveIndexMap_.at(name)].get();
    }

    virtual bool loadPrimitive(const std::string& name, Mat4 transform, const Json& prop) override {
        // Check if already loaded
        if (primitiveIndexMap_.find(name) != primitiveIndexMap_.end()) {
            LM_ERROR("Already loaded [name = '{}']", name);
            return false;
        }

        // Add a primitive
        primitiveIndexMap_[name] = primitives_.size();
        auto primitive = comp::create<Primitive>("primitive::default", this, prop);
        if (!primitive) {
            LM_ERROR("Failed to create primitive [name = '{}']", name);
            return false;
        }
        primitives_.push_back(std::move(primitive));

        return true;
    }

    virtual void build(const std::string& name) override {
        accel_ = comp::create<Accel>(name, this);
        if (!accel_) {
            return;
        }
        accel_->build(*this);
    }

    virtual std::optional<Hit> intersect(const Ray& ray, Float tmin, Float tmax) const override {
        const auto hit = accel_->intersect(ray, tmin, tmax);
        if (!hit) {
            return {};
        }
        const auto [t, uv, pid, fid] = *hit;
        // TODO: replace this
        const auto* mesh = primitives_.at(pid)->underlying("mesh");
        return Hit{
            mesh->;
        };
    }

private:
    std::vector<std::unique_ptr<Primitive>> primitives_;
    std::unordered_map<std::string, size_t> primitiveIndexMap_;
    Ptr<Accel> accel_;
};

LM_COMP_REG_IMPL(Scene_, "scene::default");

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(LM_NAMESPACE)
