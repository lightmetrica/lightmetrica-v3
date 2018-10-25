/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/api.h>
#include <lm/assets.h>
#include <lm/scene.h>

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

    }

    void shutdown() {
        
    }

    void asset(const std::string& name) {

    }

    void primitive(const std::string& name) {

    }

    void render() {

    }

private:
    Component::UniquePtr<Assets> assets_;
    Component::UniquePtr<Scene> scene_ = lm::comp::create<Scene>("scene::default");
};
}

// ----------------------------------------------------------------------------

LM_PUBLIC_API void init() {
    Context::instance().init();
}

LM_PUBLIC_API void shutdown() {
    Context::instance().shutdown();
}

LM_PUBLIC_API void asset(const std::string& name) {
    Context::instance().asset(name);
}

LM_PUBLIC_API void primitive(const std::string& name) {
    Context::instance().primitive(name);
}

LM_PUBLIC_API void render() {
    Context::instance().render();
}

LM_NAMESPACE_END(LM_NAMESPACE::api)
