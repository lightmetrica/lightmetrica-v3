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
#include <lm/film.h>
#include <lm/parallel.h>
#include <lm/progress.h>
#include <lm/debugio.h>
#include <lm/objloader.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// ------------------------------------------------------------------------------------------------

/*
    \brief User API context.
    Manages all global states manipulated by user apis.
*/
class UserContext : public Component {
private:
	bool initialized_ = false;
    Component::Ptr<Scene> scene_;
    Component::Ptr<Renderer> renderer_;

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
        if (name == "scene") {
            return scene_.get();
        }
        else if (name == "renderer") {
            return renderer_.get();
        }
        return nullptr;
    }

    void foreach_underlying(const ComponentVisitor& visit) override {
        lm::comp::visit(visit, scene_);
        lm::comp::visit(visit, renderer_);
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
        scene_ = comp::create<Scene>("scene::default", make_loc("scene"));
        assert(scene_);
        renderer_.reset();
    }

    std::string asset(const std::string& name, const std::string& impl_key, const Json& prop) {
		check_initialized();
        const auto loc = scene_->load_asset(name, impl_key, prop);
        if (!loc) {
            LM_THROW_EXCEPTION_DEFAULT(Error::IOError);
        }
        return *loc;
    }

    std::string asset(const std::string& name) {
		check_initialized();
        return "$.scene.assets." + name;
    }

    void build(const std::string& accel_name, const Json& prop) {
		check_initialized();
        scene_->build(accel_name, prop);
    }

    void renderer(const std::string& renderer_name, const Json& prop) {
		check_initialized();
        LM_INFO("Creating renderer [renderer='{}']", renderer_name);
        renderer_ = lm::comp::create<Renderer>(renderer_name, make_loc("renderer"), prop);
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
        serial::save(os, scene_);
        serial::save(os, renderer_);
    }

    void deserialize(std::istream& is) {
		check_initialized();
        LM_INFO("Loading state from stream");
        serial::load(is, scene_);
        serial::load(is, renderer_);
    }

    int root_node() {
		check_initialized();
        return scene_->root_node();
    }

    int primitive_node(const Json& prop) {
		check_initialized();
        return scene_->create_node(SceneNodeType::Primitive, prop);
    }

    int group_node() {
		check_initialized();
        return scene_->create_node(SceneNodeType::Group, {});
    }

    int instance_group_node() {
		check_initialized();
        return scene_->create_node(SceneNodeType::Group, {
            {"instanced", true}
        });
    }

    int transform_node(Mat4 transform) {
		check_initialized();
        return scene_->create_node(SceneNodeType::Group, {
            {"transform", transform}
        });
    }

    void add_child(int parent, int child) {
		check_initialized();
        scene_->add_child(parent, child);
    }

    void add_child_from_model(int parent, const std::string& model_loc) {
		check_initialized();
        scene_->add_child_from_model(parent, model_loc);
    }

    int create_group_from_model(const std::string& model_loc) {
		check_initialized();
        return scene_->create_group_from_model(model_loc);
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

LM_PUBLIC_API std::string asset(const std::string& name, const std::string& impl_key, const Json& prop) {
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

LM_PUBLIC_API int root_node() {
    return UserContext::instance().root_node();
}

LM_PUBLIC_API int primitive_node(const Json& prop) {
    return UserContext::instance().primitive_node(prop);
}

LM_PUBLIC_API int group_node() {
    return UserContext::instance().group_node();
}

LM_PUBLIC_API int instance_group_node() {
    return UserContext::instance().instance_group_node();
}

LM_PUBLIC_API int transform_node(Mat4 transform) {
    return UserContext::instance().transform_node(transform);
}

LM_PUBLIC_API void add_child(int parent, int child) {
    UserContext::instance().add_child(parent, child);
}

LM_PUBLIC_API void add_child_from_model(int parent, const std::string& model_loc) {
    UserContext::instance().add_child_from_model(parent, model_loc);
}

LM_PUBLIC_API int create_group_from_model(const std::string& model_loc) {
    return UserContext::instance().create_group_from_model(model_loc);
}

LM_PUBLIC_API void primitive(Mat4 transform, const Json& prop) {
    auto t = transform_node(transform);
    if (prop.find("model") != prop.end()) {
        add_child_from_model(t, prop["model"]);
    }
    else {
        add_child(t, primitive_node(prop));
    }
    add_child(root_node(), t);
}

LM_NAMESPACE_END(LM_NAMESPACE)
