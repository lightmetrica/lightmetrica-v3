/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/user.h>
#include <lm/assets.h>
#include <lm/scene.h>
#include <lm/renderer.h>
#include <lm/logger.h>
#include <lm/film.h>
#include <lm/parallel.h>
#include <lm/exception.h>
#include <lm/json.h>
#include <lm/progress.h>
#include <lm/serial.h>
#include <lm/debugio.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

#define THROW_RUNTIME_ERROR() \
    throw std::runtime_error("Consult log outputs for detailed error messages")

// ----------------------------------------------------------------------------

/*
    \brief User API context.
    Manages all global states manipulated by user apis.
*/
class UserContext_Default : public user::detail::UserContext {
public:
    UserContext_Default() {
        // User context is the root of the object tree
        comp::detail::registerRootComp(this);
    }

    ~UserContext_Default() {
        debugio::shutdown();
        debugio::server::shutdown();
        progress::shutdown();
        parallel::shutdown();
        log::shutdown();
        exception::shutdown();
    }

public:
    virtual bool construct(const Json& prop) override {
        // Exception subsystem
        exception::init();

        // Logger subsystem
        log::init(json::valueOr<std::string>(prop, "logger", log::DefaultType));

        // Initial message
        // This must be called after logger subsystem is initialized.
        LM_INFO("Initializing Lightmetrica [version='{}']");

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

        // Create assets and scene
        reset();

        return true;
    }

public:
    virtual Component* underlying(const std::string& name) const override {
        const auto[s, r] = comp::splitFirst(name);
        if (s == "assets") {
            return comp::getCurrentOrUnderlying(r, assets_.get());
        }
        else if (s == "scene") {
            return comp::getCurrentOrUnderlying(r, scene_.get());
        }
        else if (s == "renderer") {
            return renderer_.get();
        }
        return nullptr;
    }

    virtual void foreachUnderlying(const ComponentVisitor& visit) override {
        lm::comp::visit(visit, assets_);
        lm::comp::visit(visit, scene_);
        lm::comp::visit(visit, renderer_);
    }

public:
    virtual void reset() override {
        assets_ = comp::create<Assets>("assets::default", makeLoc(loc(), "assets"));
        assert(assets_);
        scene_ = comp::create<Scene>("scene::default", makeLoc(loc(), "scene"));
        assert(scene_);
        renderer_.reset();
    }

    virtual void asset(const std::string& name, const std::string& implKey, const Json& prop) override {
        if (!assets_->loadAsset(name, implKey, prop)) {
            THROW_RUNTIME_ERROR();
        }
    }

    virtual void primitive(Mat4 transform, const Json& prop) override {
        if (!scene_->loadPrimitive(*assets_.get(), transform, prop)) {
            THROW_RUNTIME_ERROR();
        }
    }

    virtual void primitives(Mat4 transform, const std::string& modelName) override {
        if (!scene_->loadPrimitives(*assets_.get(), transform, modelName)) {
            THROW_RUNTIME_ERROR();
        }
    }

    void build(const std::string& accelName, const Json& prop) {
        scene_->build(accelName, prop);
    }

    virtual void renderer(const std::string& rendererName, const Json& prop) override {
        renderer_ = lm::comp::create<Renderer>(rendererName, makeLoc(loc(), "renderer"), prop);
        if (!renderer_) {
            LM_ERROR("Failed to render [renderer='{}']", rendererName);
            THROW_RUNTIME_ERROR();
        }
    }

    virtual void render(bool verbose) override {
        if (verbose) {
            LM_INFO("Starting render [name='{}']", renderer_->key());
            LM_INDENT();
        }
        renderer_->render(scene_.get());
    }

    virtual void save(const std::string& filmName, const std::string& outpath) override {
        const auto* film = assets_->underlying<Film>(filmName);
        if (!film) {
            THROW_RUNTIME_ERROR();
        }
        if (!film->save(outpath)) {
            THROW_RUNTIME_ERROR();
        }
    }

    virtual FilmBuffer buffer(const std::string& filmName) override {
        auto* film = assets_->underlying<Film>(filmName);
        if (!film) {
            THROW_RUNTIME_ERROR();
        }
        return film->buffer();
    }

    virtual void serialize(std::ostream& os) override {
        LM_INFO("Saving state to stream");
        serial::save(os, assets_);
        serial::save(os, scene_);
        serial::save(os, renderer_);
    }

    virtual void deserialize(std::istream& is) override {
        LM_INFO("Loading state from stream");
        serial::load(is, assets_);
        serial::load(is, scene_);
        serial::load(is, renderer_);
    }

private:
    Component::Ptr<Assets> assets_;
    Component::Ptr<Scene> scene_;
    Component::Ptr<Renderer> renderer_;
};

LM_COMP_REG_IMPL(UserContext_Default, "user::default");

// ----------------------------------------------------------------------------

using Instance = comp::detail::ContextInstance<user::detail::UserContext>;

LM_PUBLIC_API void init(const std::string& type, const Json& prop) {
    Instance::init(type, prop);
}

LM_PUBLIC_API void shutdown() {
    Instance::shutdown();
}

LM_PUBLIC_API void reset() {
    Instance::get().reset();
}

LM_PUBLIC_API void asset(const std::string& name, const std::string& implKey, const Json& prop) {
    Instance::get().asset(name, implKey, prop);
}

LM_PUBLIC_API void primitive(Mat4 transform, const Json& prop) {
    Instance::get().primitive(transform, prop);
}

LM_PUBLIC_API void primitives(Mat4 transform, const std::string& modelName) {
    Instance::get().primitives(transform, modelName);
}

LM_PUBLIC_API void build(const std::string& accelName, const Json& prop) {
    Instance::get().build(accelName, prop);
}

LM_PUBLIC_API void renderer(const std::string& rendererName, const Json& prop) {
    Instance::get().renderer(rendererName, prop);
}

LM_PUBLIC_API void render(bool verbose) {
    Instance::get().render(verbose);
}

LM_PUBLIC_API void save(const std::string& filmName, const std::string& outpath) {
    Instance::get().save(filmName, outpath);
}

LM_PUBLIC_API FilmBuffer buffer(const std::string& filmName) {
    return Instance::get().buffer(filmName);
}

LM_PUBLIC_API void serialize(std::ostream& os) {
    Instance::get().serialize(os);
}

LM_PUBLIC_API void deserialize(std::istream& is) {
    Instance::get().deserialize(is);
}

LM_NAMESPACE_END(LM_NAMESPACE)
