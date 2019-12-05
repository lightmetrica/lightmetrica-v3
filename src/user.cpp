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
#include <lm/film.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*
    User API context.
    Manages all global states manipulated by user apis.
*/
class UserContext : public Component {
private:
	bool initialized_ = false;
	Ptr<Assets> assets_;        // Underlying assets
    Ptr<Scene> scene_;          // Underlying scene
    Ptr<Renderer> renderer_;    // Underlying renderer

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
    void init(const Json& prop) {
        // Exception subsystem
        exception::init();

        // Logger subsystem
        log::init(json::value<std::string>(prop, "logger", log::DefaultType));

        // Parallel subsystem
        parallel::init("parallel::openmp", prop);
        if (auto it = prop.find("progress");  it != prop.end()) {
            it = it->begin();
            progress::init(it.key(), it.value());
        }
        else {
            progress::init(progress::DefaultType);
        }

        // Debugio subsystem
        // This subsystem is initialized only if the parameter is given
        if (auto it = prop.find("debugio"); it != prop.end()) {
            it = it->begin();
            debugio::init(it.key(), it.value());
        }
        if (auto it = prop.find("debugio_server"); it != prop.end()) {
            it = it->begin();
            debugio::server::init(it.key(), it.value());
        }

        // OBJ loader
        objloader::init();

        // Create assets and scene
        reset();

		// Initialized
		initialized_ = true;
    }

    void shutdown() {
		check_initialized();
        objloader::shutdown();
        debugio::shutdown();
        debugio::server::shutdown();
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
        else if (name == "scene") {
            return scene_.get();
        }
        else if (name == "renderer") {
            return renderer_.get();
        }
        return nullptr;
    }

    void foreach_underlying(const ComponentVisitor& visit) override {
        comp::visit(visit, assets_);
        comp::visit(visit, scene_);
        comp::visit(visit, renderer_);
    }

public:
    void info() {
		check_initialized();
        // Print information of Lightmetrica
        LM_INFO("Lightmetrica -- Version {} {} {}",
            version::formatted(),
            version::platform(),
            version::architecture());
    }

    void reset() {
        assets_ = comp::create<Assets>("assets::default", make_loc("assets"));
        scene_ = comp::create<Scene>("scene::default", make_loc("scene"));
        assert(scene_);
        renderer_.reset();
    }

    Component* asset(const std::string& name, const std::string& impl_key, const Json& prop) {
		check_initialized();
        const auto p = assets_->load_asset(name, impl_key, prop);
        if (!p) {
            LM_THROW_EXCEPTION_DEFAULT(Error::IOError);
        }
        return p;
    }

    std::string asset(const std::string& name) {
		check_initialized();
        return "$.assets." + name;
    }

    void build(const std::string& accel_name, const Json& prop) {
		check_initialized();
        scene_->build(accel_name, prop);
    }

    void renderer(const std::string& renderer_name, const Json& prop) {
		check_initialized();
        LM_INFO("Creating renderer [renderer='{}']", renderer_name);
        renderer_ = comp::create<Renderer>(renderer_name, make_loc("renderer"), prop);
        if (!renderer_) {
            LM_THROW_EXCEPTION_DEFAULT(Error::FailedToRender);
        }
    }

    void render(bool verbose) {
		check_initialized();
        if (verbose) {
            LM_INFO("Starting render [name='{}']", renderer_->key());
            LM_INDENT();
        }
        renderer_->render(scene_.get());
    }

    void save(const std::string& film_name, const std::string& outpath) {
		check_initialized();
        const auto* film = comp::get<Film>(film_name);
        if (!film) {
            LM_THROW_EXCEPTION_DEFAULT(Error::IOError);
        }
        if (!film->save(outpath)) {
            LM_THROW_EXCEPTION_DEFAULT(Error::IOError);
        }
    }

    FilmBuffer buffer(const std::string& film_name) {
		check_initialized();
        auto* film = comp::get<Film>(film_name);
        if (!film) {
            LM_THROW_EXCEPTION_DEFAULT(Error::IOError);
        }
        return film->buffer();
    }

    void serialize(std::ostream& os) {
		check_initialized();
        LM_INFO("Saving state to stream");
        serial::save(os, assets_);
        serial::save(os, scene_);
        serial::save(os, renderer_);
    }

    void deserialize(std::istream& is) {
		check_initialized();
        LM_INFO("Loading state from stream");
        serial::load(is, assets_);
        serial::load(is, scene_);
        serial::load(is, renderer_);
    }

	Scene* scene() {
		return scene_.get();
	}

	void primitive(Mat4 transform, const Json& prop) {
		check_initialized();
		auto t = scene_->create_group_node(transform);
		if (prop.find("model") != prop.end()) {
			scene_->add_child_from_model(t, prop["model"]);
		}
		else {
			scene_->add_child(t, scene_->create_primitive_node(prop));
		}
		scene_->add_child(scene_->root_node(), t);
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

LM_PUBLIC_API Component* asset(const std::string& name, const std::string& impl_key, const Json& prop) {
    return UserContext::instance().asset(name, impl_key, prop);
}

LM_PUBLIC_API std::string asset(const std::string& name) {
	return UserContext::instance().asset(name);
}

LM_PUBLIC_API void build(const std::string& accel_name, const Json& prop) {
    UserContext::instance().build(accel_name, prop);
}

LM_PUBLIC_API void renderer(const std::string& renderer_name, const Json& prop) {
    UserContext::instance().renderer(renderer_name, prop);
}

LM_PUBLIC_API void render(bool verbose) {
    UserContext::instance().render(verbose);
}

LM_PUBLIC_API void save(const std::string& filmName, const std::string& outpath) {
    UserContext::instance().save(filmName, outpath);
}

LM_PUBLIC_API FilmBuffer buffer(const std::string& filmName) {
    return UserContext::instance().buffer(filmName);
}

LM_PUBLIC_API void serialize(std::ostream& os) {
    UserContext::instance().serialize(os);
}

LM_PUBLIC_API void deserialize(std::istream& is) {
    UserContext::instance().deserialize(is);
}

LM_PUBLIC_API Scene* scene() {
	return UserContext::instance().scene();
}

LM_PUBLIC_API void primitive(Mat4 transform, const Json& prop) {
	UserContext::instance().primitive(transform, prop);
}

LM_NAMESPACE_END(LM_NAMESPACE)
