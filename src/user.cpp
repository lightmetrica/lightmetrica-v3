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
#include <lm/debugio.h>
#include <lm/objloader.h>
#include <lm/assets.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*
    User API context.
    Manages all global states manipulated by user apis.
*/
class UserContext : public Component {
private:
	bool initialized_ = false;  // True if initialized
	Ptr<Assets> assets_;        // Underlying assets

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
            return assets_.get();
        }
        return nullptr;
    }

    void foreach_underlying(const ComponentVisitor& visit) override {
        comp::visit(visit, assets_);
    }

public:
    void info() {
		check_initialized();
        LM_INFO("Lightmetrica -- Version {} {} {}",
            version::formatted(),
            version::platform(),
            version::architecture());
    }

    Assets* assets() {
        return assets_.get();
    }

    void reset() {
        // Initialize asset
        assets_ = comp::create<Assets>("assets::default", make_loc("assets"));
    }

    void serialize(std::ostream& os) {
		check_initialized();
        LM_INFO("Saving state to stream");
        serial::save(os, assets_);
    }

    void deserialize(std::istream& is) {
		check_initialized();
        LM_INFO("Loading state from stream");
        serial::load(is, assets_);
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

LM_PUBLIC_API Assets* assets() {
    return UserContext::instance().assets();
}

LM_PUBLIC_API void serialize(std::ostream& os) {
    UserContext::instance().serialize(os);
}

LM_PUBLIC_API void deserialize(std::istream& is) {
    UserContext::instance().deserialize(is);
}

LM_NAMESPACE_END(LM_NAMESPACE)
