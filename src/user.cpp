/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/version.h>
#include <lm/user.h>
#include <lm/scene.h>
#include <lm/renderer.h>
#include <lm/parallel.h>
#include <lm/progress.h>
#include <lm/objloader.h>
#include <lm/assetgroup.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*
    User API context.
    Manages all global states manipulated by user apis.
*/
class UserContext : public Component {
private:
	bool initialized_ = false;      // True if initialized
	Ptr<AssetGroup> root_assets_;   // Root asset group.

public:
    UserContext() {
        // User context is the root of the object tree.
        // Root locator is '$'.
        comp::detail::Access::loc(this) = "$";
        comp::detail::register_root_comp(this);
    }

public:
    static UserContext& instance() {
        static UserContext instance;
        return instance;
    }

private:
	// Check if the user context is initialized.
	// If not initialize, this function throws an exception.
	void check_initialized() const {
		if (initialized_) {
			return;
		}
		LM_THROW_EXCEPTION(Error::Uninitialized,
			"Lightmetrica is not initialize. Call lm::init() function.");
	}

public:
    void init(const Json&) {
        exception::init();
        log::init();
        parallel::init();
        progress::init();
        objloader::init();
        reset();
		initialized_ = true;
    }

    void shutdown() {
		check_initialized();
        objloader::shutdown();
        progress::shutdown();
        parallel::shutdown();
        log::shutdown();
        exception::shutdown();
		initialized_ = false;
    }

public:
    Component* underlying(const std::string& name) const override {
        if (name == "assets") {
            return root_assets_.get();
        }
        return nullptr;
    }

    void foreach_underlying(const ComponentVisitor& visit) override {
        comp::visit(visit, root_assets_);
    }

public:
    void info() {
		check_initialized();
        LM_INFO("Lightmetrica -- Version {} {} {}",
            version::formatted(),
            version::platform(),
            version::architecture());
    }

    AssetGroup* assets() {
        return root_assets_.get();
    }

    void reset() {
        // Initialize asset
        root_assets_ = comp::create<AssetGroup>("asset_group::default", make_loc("assets"));
    }

    void save_state_to_file(const std::string& path) {
        std::ofstream os(path, std::ios::out | std::ios::binary);
        serial::save_comp(os, root_assets_, root_assets_->loc());
    }

    void load_state_from_file(const std::string& path) {
        std::ifstream is(path, std::ios::in | std::ios::binary);
        serial::load_comp(is, root_assets_, root_assets_->loc());
    }
};

// ------------------------------------------------------------------------------------------------

LM_PUBLIC_API void init(const Json& prop) {
    UserContext::instance().init(prop);
}

LM_PUBLIC_API void shutdown() {
    UserContext::instance().shutdown();
}

LM_PUBLIC_API void reset() {
    UserContext::instance().reset();
}

LM_PUBLIC_API void info() {
    UserContext::instance().info();
}

LM_PUBLIC_API AssetGroup* assets() {
    return UserContext::instance().assets();
}

LM_PUBLIC_API void save_state_to_file(const std::string& path) {
    UserContext::instance().save_state_to_file(path);
}

LM_PUBLIC_API void load_state_from_file(const std::string& path) {
    UserContext::instance().load_state_from_file(path);
}

LM_NAMESPACE_END(LM_NAMESPACE)
