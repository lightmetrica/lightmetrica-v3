/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/assets.h>
#include <lm/logger.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Assets_ final : public Assets {
public:
    virtual Component* underlying(const std::string& name) const override {
        // Finds underlying asset
        auto it = assetIndexMap_.find(name);
        if (it == assetIndexMap_.end()) {
            LM_ERROR("Asset [name = '{}'] is not found", name);
            return nullptr;
        }
        return assets_.at(it->second).get();
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
        auto p = comp::create<Component>(implKey, this);
        if (!p) {
            LM_ERROR("Failed to create component [name='{}', key='{}']", name, implKey);
            return false;
        }
        if (!p->construct(prop)) {
            LM_ERROR("Failed to initialize component [name='{}', key='{}']", name, implKey);
            return false;
        }

        // Register created asset
        assetIndexMap_[name] = int(assets_.size());
        assets_.push_back(std::move(p));

        return true;
    }

private:
    std::vector<Component::Ptr<Component>> assets_;
    std::unordered_map<std::string, int> assetIndexMap_;
};

LM_COMP_REG_IMPL(Assets_, "assets::default");

LM_NAMESPACE_END(LM_NAMESPACE)
