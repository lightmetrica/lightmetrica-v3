/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/scene.h>
#include <lm/assets.h>
#include <lm/accel.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Scene_ final : public Scene {
public:
    virtual bool loadPrimitive(const std::string& name, const Assets& assets, mat4 transform, const Json& prop) override {
        // Check if already loaded
        if (primitiveIndexMap_.find(name) != primitiveIndexMap_.end()) {
            return false;
        }
        
        // Function to find and obtain an asset by name
        const auto getAssetRefBy = [&](const std::string& propName) -> const Component* {
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

        // Add a primitive
        primitiveIndexMap_[name] = primitives_.size();
        primitives_.push_back(ScenePrimitive{
            transform,
            getAssetRefBy("mesh"),
            getAssetRefBy("material"),
            getAssetRefBy("sensor"),
            getAssetRefBy("light")
        });

        return true;
    }

    virtual void build(const std::string& name) override {
        accel_ = comp::create<Accel>(name);
        if (!accel_) {
            return;
        }
        accel_->build(*this);
    }

private:
    std::vector<ScenePrimitive> primitives_;
    std::unordered_map<std::string, size_t> primitiveIndexMap_;
    Ptr<Accel> accel_;
};

LM_COMP_REG_IMPL(Scene_, "scene::default");

LM_NAMESPACE_END(LM_NAMESPACE)
