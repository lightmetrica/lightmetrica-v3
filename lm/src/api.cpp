/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/api.h>
#include <lm/assets.h>
#include <lm/scene.h>
#include <lm/logger.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE::api)

// ----------------------------------------------------------------------------

namespace {

/*
    \brief User API context.
    Manages all global states manipulated by user apis.
*/
class Context {
public:
    static Context& instance() {
        static Context instance;
        return instance;
    }
    
public:
    void init() {
        log::init();
    }

    void shutdown() {
        log::shutdown();
    }

    void asset(const std::string& name, const std::string& implKey, const json& prop) {
        assets_->loadAsset(name, implKey, prop);
    }

    void primitive(const std::string& name, mat4 transform, const json& prop) {

    }

    void render() {

    }

private:
    Component::Ptr<Assets> assets_ = comp::create<Assets>("assets::default");
    Component::Ptr<Scene> scene_ = comp::create<Scene>("scene::default");
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

LM_PUBLIC_API void primitive(const std::string& name, glm::mat4 transform, const Component* mesh, const Component* material) {
    Context::instance().primitive(name, transform, mesh, material);
}

LM_PUBLIC_API void render() {
    Context::instance().render();
}

LM_NAMESPACE_END(LM_NAMESPACE::api)
