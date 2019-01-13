/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/assets.h>
#include <lm/logger.h>
#include <lm/serial.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Assets_ final : public Assets {
private:
    std::vector<Component::Ptr<Component>> assets_;
    std::unordered_map<std::string, int> assetIndexMap_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(assetIndexMap_, assets_);
    }

public:
    virtual Component* underlying(const std::string& name) const override {
        // Take first element inside `name`
        const auto [s, r] = comp::splitFirst(name);

        // Finds underlying asset
        auto it = assetIndexMap_.find(s);
        if (it == assetIndexMap_.end()) {
            LM_ERROR("Asset [name = '{}'] is not found", s);
            return nullptr;
        }

        // Try to find nested asset. If not found, return as it is.
        auto* comp = assets_.at(it->second).get();
        return r.empty() ? comp : comp->underlying(r);
    }

    virtual bool loadAsset(const std::string& name, const std::string& implKey, const Json& prop) override {
        LM_INFO("Loading asset [name='{}']", name);
        LM_INDENT();

        // Check if the asset with given name has been already loaded
        if (assetIndexMap_.find(name) != assetIndexMap_.end()) {
            LM_ERROR("Asset [name='{}'] has been already loaded", name);
            return false;
        }

        // Create an instance of the asset
        auto p = comp::create<Component>(implKey, makeLoc(loc(), name));
        if (!p) {
            LM_ERROR("Failed to create component [name='{}', key='{}']", name, implKey);
            return false;
        }

        // Register created asset
        // Note that this must happen before construct() because
        // the instance could be accessed by underlying() while initialization.
        assetIndexMap_[name] = int(assets_.size());
        assets_.push_back(std::move(p));

        // Initialize the asset
        if (!assets_.back()->construct(prop)) {
            LM_ERROR("Failed to initialize component [name='{}', key='{}']", name, implKey);
            assets_.pop_back();
            return false;
        }

        return true;
    }
};

LM_COMP_REG_IMPL(Assets_, "assets::default");

LM_NAMESPACE_END(LM_NAMESPACE)
