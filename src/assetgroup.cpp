/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/assetgroup.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class AssetGroup_ final : public AssetGroup {
private:
    std::vector<Ptr<Component>> assets_;
    std::unordered_map<std::string, int> asset_index_map_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(asset_index_map_, assets_);
    }

    virtual void foreach_underlying(const ComponentVisitor& visitor) override {
        for (auto& asset : assets_) {
            comp::visit(visitor, asset);
        }
    }

    virtual Component* underlying(const std::string& name) const override {
        auto it = asset_index_map_.find(name);
        if (it == asset_index_map_.end()) {
            LM_ERROR("Invalid asset name [name='{}']", name);
            return nullptr;
        }
        return assets_.at(it->second).get();
    }

private:
    bool valid_asset_name(const std::string& name) const {
        std::regex regex(R"x([:\w_-]+)x");
        std::smatch match;
        return std::regex_match(name, match, regex);
    }

public:
    virtual Component* load_asset(const std::string& name, const std::string& impl_key, const Json& prop) override {
        LM_INFO("Loading asset [name='{}']", name);
        LM_INDENT();

        // Check if asset name is valid
        if (!valid_asset_name(name)) {
            LM_ERROR("Invalid asset name [name='{}']", name);
            return nullptr;
        }

        // Check if the asset with given name has been already loaded
        const auto it = asset_index_map_.find(name);
        const bool found = it != asset_index_map_.end();
        if (found) {
            LM_INFO("Asset [name='{}'] has been already loaded. Replacing..", name);
        }

        // Create an instance of the asset
        auto p = comp::create_without_construct<Component>(impl_key, make_loc(loc(), name));
        if (!p) {
            LM_ERROR("Failed to create an asset [name='{}', key='{}']", name, impl_key);
            return nullptr;
        }

        // Register created asset
        // Note that the registration must happen before construct() because
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

            // Initialize the asset
            // This might cause an exception
            asset->construct(prop);

            // Notify to update the weak references in the object tree
            const Component::ComponentVisitor visitor = [&](Component*& comp, bool weak) {
                if (!comp) {
                    return;
                }
                if (asset == comp) {
                    // Ignore myself
                    return;
                }
                if (!weak) {
                    comp->foreach_underlying(visitor);
                }
                else {
                    comp::update_weak_ref(comp);
                }
            };
            comp::get<Component>("$")->foreach_underlying(visitor);
        }
        else {
            // Register as a new asset
            asset_index_map_[name] = int(assets_.size());
            assets_.push_back(std::move(p));
            asset = assets_.back().get();

            // Initialize the asset
            asset->construct(prop);
        }

        return asset;
    }

    virtual Component* load_serialized(const std::string& name, const std::string& path) override {
        LM_INFO("Loading serialized asset [name='{}']", name);
        LM_INDENT();

        if (asset_index_map_.find(name) != asset_index_map_.end()) {
            LM_THROW_EXCEPTION(Error::InvalidArgument,
                "Asset [name='{}'] has been already loaded.", name);
        }

        Ptr<Component> p;
        asset_index_map_[name] = int(assets_.size());
        assets_.emplace_back();
        serial::load(path, assets_.back());
        return assets_.back().get();
    }
};

LM_COMP_REG_IMPL(AssetGroup_, "asset_group::default");

LM_NAMESPACE_END(LM_NAMESPACE)
