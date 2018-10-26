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
        // Initialize logger system
        log::init();
    }

    void shutdown() {
        // Shutdown logger system
        log::shutdown();
    }

    void asset(const std::string& name, const json& params) {

    }

    void primitive(const std::string& name) {

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

LM_PUBLIC_API void asset(const std::string& name, const json& params) {
    Context::instance().asset(name, params);
}

LM_PUBLIC_API void primitive(const std::string& name) {
    Context::instance().primitive(name);
}

LM_PUBLIC_API void render() {
    Context::instance().render();
}

LM_NAMESPACE_END(LM_NAMESPACE::api)
