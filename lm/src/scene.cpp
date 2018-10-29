/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/scene.h>
#include <lm/assets.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Scene_ final : public Scene {
public:
    virtual bool loadPrimitive(const std::string& name, const Assets& assets, mat4 transform, const json& prop) override {
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
        primitives_.emplace_back(new ScenePrimitive{
            transform,
            getAssetRefBy("mesh"),
            getAssetRefBy("material"),
            getAssetRefBy("sensor"),
            getAssetRefBy("light")
        });

        return true;
    }

private:
    std::vector<std::unique_ptr<ScenePrimitive>> primitives_;
    std::unordered_map<std::string, size_t> primitiveIndexMap_;
};

LM_COMP_REG_IMPL(Scene_, "scene::default");

LM_NAMESPACE_END(LM_NAMESPACE)
