/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/assets.h>
#include <lm/logger.h>
#include <lm/serial.h>
#include <lm/user.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Assets_ final : public Assets {
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

private:
    bool validAssetName(const std::string& name) const {
        std::regex regex(R"x([:\w_-]+)x");
        std::smatch match;
        return std::regex_match(name, match, regex);
    }

public:
    virtual Component* underlying(const std::string& name) const override {
        return assets_.at(assetIndexMap_.at(name)).get();
    }

    virtual std::optional<std::string> loadAsset(const std::string& name, const std::string& implKey, const Json& prop) override {
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
        auto p = comp::create<Component>(implKey, makeLoc(loc(), name));
        if (!p) {
            LM_ERROR("Failed to create component [name='{}', key='{}']", name, implKey);
            return {};
        }

        // Register created asset
        // Note that this must happen before construct() because
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

            // Notify to update the weak references in the object tree
            const lm::Component::ComponentVisitor visit = [&](lm::Component*& comp, bool weak) {
                if (!comp) {
                    return;
                }
                if (asset == comp) {
                    // Ignore myself
                    return;
                }
                if (!weak) {
                    comp->foreachUnderlying(visit);
                }
                else {
                    comp::updateWeakRef(comp);
                }
            };
            comp::detail::foreachComponent(visit);
        }
        else {
            // Register as a new asset
            assetIndexMap_[name] = int(assets_.size());
            assets_.push_back(std::move(p));
            asset = assets_.back().get();
        }

        // Initialize the asset
        if (!asset->construct(prop)) {
            LM_ERROR("Failed to initialize component [name='{}', key='{}']", name, implKey);
            assets_.pop_back();
            return {};
        }

        return asset->loc();
    }
};

LM_COMP_REG_IMPL(Assets_, "assets::default");

LM_NAMESPACE_END(LM_NAMESPACE)
