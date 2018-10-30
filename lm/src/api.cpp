/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/api.h>
#include <lm/assets.h>
#include <lm/scene.h>
#include <lm/renderer.h>
#include <lm/logger.h>
#include <lm/film.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE::api)

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
    

public:
    void init() {
        log::init();
        if (init_) {
            LM_WARN("Lightmetrica is already initialized");
            return;
        }
        assets_ = comp::create<Assets>("assets::default");
        scene_  = comp::create<Scene>("scene::default");
        init_ = true;
    }

    void shutdown() {
        if (!checkInitialized()) { return; }
        renderer_.reset();
        scene_.reset();
        assets_.reset();
        log::shutdown();
    }

    void asset(const std::string& name, const std::string& implKey, const json& prop) {
        if (!checkInitialized()) { return; }
        assets_->loadAsset(name, implKey, prop);
    }

    void primitive(const std::string& name, mat4 transform, const json& prop) {
        if (!checkInitialized()) { return; }
        scene_->loadPrimitive(name, *assets_.get(), transform, prop);
    }

    const ScenePrimitive* primitive(const std::string& name) {
        if (!checkInitialized()) { return nullptr; }
        return cast<ScenePrimitive>(scene_->underlying(name));
    }

    void render(const std::string& rendererName, const std::string& accelName, const json& prop) {
        if (!checkInitialized()) { return; }
        scene_->build(accelName);
        renderer_ = lm::comp::create<Renderer>(rendererName, prop, this);
        renderer_->render(*scene_.get());
    }

    void save(const std::string& filmName, const std::string& outpath) {
        if (!checkInitialized()) { return; }
        const auto* film = assets_->underlying<Film>(filmName);
        if (!film) {
            return;
        }
        if (!film->save(outpath)) {
            return;
        }
    }

private:
    bool checkInitialized() {
        if (!init_) {
            LM_ERROR("Lightmetrica is not initialized");
            return false;
        }
        return true;
    }

private:
    bool init_ = false;
    Component::Ptr<Assets> assets_;
    Component::Ptr<Scene> scene_;
    Component::Ptr<Renderer> renderer_;
};

}

// ----------------------------------------------------------------------------

LM_PUBLIC_API void init() {
    Context::instance().init();
}

LM_PUBLIC_API void shutdown() {
    Context::instance().shutdown();
}

LM_PUBLIC_API void asset(const std::string& name, const std::string& implKey, const json& prop) {
    Context::instance().asset(name, implKey, prop);
}

LM_PUBLIC_API void primitive(const std::string& name, mat4 transform, const json& prop) {
    Context::instance().primitive(name, transform, prop);
}

LM_PUBLIC_API const ScenePrimitive* primitive(const std::string& name) {
    return Context::instance().primitive(name);
}

LM_PUBLIC_API void render(const std::string& rendererName, const std::string& accelName, const json& prop) {
    Context::instance().render(rendererName, accelName, prop);
}

LM_PUBLIC_API void save(const std::string& filmName, const std::string& outpath) {
    Context::instance().save(filmName, outpath);
}

LM_NAMESPACE_END(LM_NAMESPACE::api)
