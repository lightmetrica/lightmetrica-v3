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

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

#define THROW_RUNTIME_ERROR() \
    throw std::runtime_error("Consult log outputs for detailed error messages")

// ----------------------------------------------------------------------------

namespace {

/*
    \brief User API context.
    Manages all global states manipulated by user apis.
*/
class Context : public Component {
public:
    static Context& instance() {
        static Context instance;
        return instance;
    }

public:
    virtual Component* underlying(const std::string& name) const {
        const auto [s, r] = comp::splitFirst(name);
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

public:
    void init(const Json& prop) {
        exception::init();
        log::init(valueOr<std::string>(prop, "logger", log::DefaultType));
        LM_INFO("Initializing Lightmetrica [version='{}']");
        parallel::init("parallel::openmp", prop);
        {
            auto it = prop.find("progress");
            if (it != prop.end()) {
                it = it->begin();
                progress::init(it.key(), it.value());
            }
            else {
                progress::init(progress::DefaultType);
            }
        }
        //progress::init(valueOr<std::string>(prop, "progress", progress::DefaultType));
        assets_ = comp::create<Assets>("assets::default", this);
        scene_  = comp::create<Scene>("scene::default", this);
    }

    void shutdown() {
        renderer_.reset();
        scene_.reset();
        assets_.reset();
        progress::shutdown();
        parallel::shutdown();
        log::shutdown();
        exception::shutdown();
    }

    void asset(const std::string& name, const std::string& implKey, const Json& prop) {
        if (!assets_->loadAsset(name, implKey, prop)) {
            THROW_RUNTIME_ERROR();
        }
    }

    Component* getAsset(const std::string& name) {
        return assets_->underlying(name);
    }

    void primitive(Mat4 transform, const Json& prop) {
        if (!scene_->loadPrimitive(*assets_.get(), transform, prop)) {
            THROW_RUNTIME_ERROR();
        }
    }

    void primitives(Mat4 transform, const std::string& modelName) {
        if (!scene_->loadPrimitives(*assets_.get(), transform, modelName)) {
            THROW_RUNTIME_ERROR();
        }
    }

    void build(const std::string& accelName) {
        scene_->build(accelName);
    }

    void render(const std::string& rendererName, const Json& prop) {
        renderer_ = lm::comp::create<Renderer>(rendererName, this, prop);
        if (!renderer_) {
            LM_ERROR("Failed to render [renderer='{}']", rendererName);
            THROW_RUNTIME_ERROR();
        }
        LM_INFO("Starting render [name='{}']", rendererName);
        LM_INDENT();
        renderer_->render(scene_.get());
    }

    void save(const std::string& filmName, const std::string& outpath) {
        const auto* film = assets_->underlying<Film>(filmName);
        if (!film) {
            THROW_RUNTIME_ERROR();
        }
        if (!film->save(outpath)) {
            THROW_RUNTIME_ERROR();
        }
    }

    FilmBuffer buffer(const std::string& filmName) {
        auto* film = assets_->underlying<Film>(filmName);
        if (!film) {
            THROW_RUNTIME_ERROR();
        }
        return film->buffer();
    }

    void serialize(const std::string& path) {
        LM_UNUSED(path);
        LM_TBA_RUNTIME();
    }

    void deserialize(const std::string& path) {
        LM_UNUSED(path);
        LM_TBA_RUNTIME();
    }

private:
    Component::Ptr<Assets> assets_;
    Component::Ptr<Scene> scene_;
    Component::Ptr<Renderer> renderer_;
};

}

// ----------------------------------------------------------------------------

LM_PUBLIC_API void init(const Json& prop) {
    Context::instance().init(prop);
}

LM_PUBLIC_API void shutdown() {
    Context::instance().shutdown();
}

LM_PUBLIC_API void asset(const std::string& name, const std::string& implKey, const Json& prop) {
    Context::instance().asset(name, implKey, prop);
}

LM_PUBLIC_API Component* getAsset(const std::string& name) {
    return Context::instance().getAsset(name);
}

LM_PUBLIC_API void primitive(Mat4 transform, const Json& prop) {
    Context::instance().primitive(transform, prop);
}

LM_PUBLIC_API void primitives(Mat4 transform, const std::string& modelName) {
    Context::instance().primitives(transform, modelName);
}

LM_PUBLIC_API void build(const std::string& accelName) {
    Context::instance().build(accelName);
}

LM_PUBLIC_API void render(const std::string& rendererName, const Json& prop) {
    Context::instance().render(rendererName, prop);
}

LM_PUBLIC_API void save(const std::string& filmName, const std::string& outpath) {
    Context::instance().save(filmName, outpath);
}

LM_PUBLIC_API FilmBuffer buffer(const std::string& filmName) {
    return Context::instance().buffer(filmName);
}

LM_PUBLIC_API void serialize(const std::string& path) {
    Context::instance().serialize(path);
}

LM_PUBLIC_API void deserialize(const std::string& path) {
    Context::instance().deserialize(path);
}

LM_NAMESPACE_END(LM_NAMESPACE)
