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
    Component::Ptr<Scene> scene_;
    Component::Ptr<Renderer> renderer_;

public:
    UserContext() {
        // User context is the root of the object tree.
        // Root locator is '$'.
        comp::detail::Access::loc(this) = "$";
        comp::detail::registerRootComp(this);
    }

public:
    static UserContext& instance() {
        static UserContext instance;
        return instance;
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
    }

    void shutdown() {
        objloader::shutdown();
        debugio::shutdown();
        debugio::server::shutdown();
        progress::shutdown();
        parallel::shutdown();
        log::shutdown();
        exception::shutdown();
    }

public:
    Component* underlying(const std::string& name) const {
        if (name == "scene") {
            return scene_.get();
        }
        else if (name == "renderer") {
            return renderer_.get();
        }
        return nullptr;
    }

    void foreachUnderlying(const ComponentVisitor& visit) {
        lm::comp::visit(visit, scene_);
        lm::comp::visit(visit, renderer_);
    }

public:
    void info() {
        // Print information of Lightmetrica
        LM_INFO("Lightmetrica -- Version {} {} {}",
            version::formatted(),
            version::platform(),
            version::architecture());
    }

    void reset() {
        scene_ = comp::create<Scene>("scene::default", makeLoc("scene"));
        assert(scene_);
        renderer_.reset();
    }

    std::string asset(const std::string& name, const std::string& implKey, const Json& prop) {
        const auto loc = scene_->loadAsset(name, implKey, prop);
        if (!loc) {
            LM_THROW_EXCEPTION_DEFAULT(Error::IOError);
        }
        return *loc;
    }

    std::string asset(const std::string& name) {
        return "$.scene.assets." + name;
    }

    void build(const std::string& accelName, const Json& prop) {
        scene_->build(accelName, prop);
    }

    void renderer(const std::string& rendererName, const Json& prop) {
        LM_INFO("Creating renderer [renderer='{}']", rendererName);
        renderer_ = lm::comp::create<Renderer>(rendererName, makeLoc("renderer"), prop);
        if (!renderer_) {
            LM_THROW_EXCEPTION_DEFAULT(Error::FailedToRender);
        }
    }

    void render(bool verbose) {
        if (verbose) {
            LM_INFO("Starting render [name='{}']", renderer_->key());
            LM_INDENT();
        }
        renderer_->render(scene_.get());
    }

    void save(const std::string& filmName, const std::string& outpath) {
        const auto* film = comp::get<Film>(filmName);
        if (!film) {
            LM_THROW_EXCEPTION_DEFAULT(Error::IOError);
        }
        if (!film->save(outpath)) {
            LM_THROW_EXCEPTION_DEFAULT(Error::IOError);
        }
    }

    FilmBuffer buffer(const std::string& filmName) {
        auto* film = comp::get<Film>(filmName);
        if (!film) {
            LM_THROW_EXCEPTION_DEFAULT(Error::IOError);
        }
        return film->buffer();
    }

    void serialize(std::ostream& os) {
        LM_INFO("Saving state to stream");
        serial::save(os, scene_);
        serial::save(os, renderer_);
    }

    void deserialize(std::istream& is) {
        LM_INFO("Loading state from stream");
        serial::load(is, scene_);
        serial::load(is, renderer_);
    }

    int rootNode() {
        return scene_->rootNode();
    }

    int primitiveNode(const Json& prop) {
        return scene_->createNode(SceneNodeType::Primitive, prop);
    }

    int groupNode() {
        return scene_->createNode(SceneNodeType::Group, {});
    }

    int instanceGroupNode() {
        return scene_->createNode(SceneNodeType::Group, {
            {"instanced", true}
        });
    }

    int transformNode(Mat4 transform) {
        return scene_->createNode(SceneNodeType::Group, {
            {"transform", transform}
        });
    }

    void addChild(int parent, int child) {
        scene_->addChild(parent, child);
    }

    void addChildFromModel(int parent, const std::string& modelLoc) {
        scene_->addChildFromModel(parent, modelLoc);
    }

    int createGroupFromModel(const std::string& modelLoc) {
        return scene_->createGroupFromModel(modelLoc);
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

LM_PUBLIC_API std::string asset(const std::string& name, const std::string& implKey, const Json& prop) {
    return UserContext::instance().asset(name, implKey, prop);
}

LM_PUBLIC_API std::string asset(const std::string& name) {
    return UserContext::instance().asset(name);
}

LM_PUBLIC_API void build(const std::string& accelName, const Json& prop) {
    UserContext::instance().build(accelName, prop);
}

LM_PUBLIC_API void renderer(const std::string& rendererName, const Json& prop) {
    UserContext::instance().renderer(rendererName, prop);
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

LM_PUBLIC_API int rootNode() {
    return UserContext::instance().rootNode();
}

LM_PUBLIC_API int primitiveNode(const Json& prop) {
    return UserContext::instance().primitiveNode(prop);
}

LM_PUBLIC_API int groupNode() {
    return UserContext::instance().groupNode();
}

LM_PUBLIC_API int instanceGroupNode() {
    return UserContext::instance().instanceGroupNode();
}

LM_PUBLIC_API int transformNode(Mat4 transform) {
    return UserContext::instance().transformNode(transform);
}

LM_PUBLIC_API void addChild(int parent, int child) {
    UserContext::instance().addChild(parent, child);
}

LM_PUBLIC_API void addChildFromModel(int parent, const std::string& modelLoc) {
    UserContext::instance().addChildFromModel(parent, modelLoc);
}

LM_PUBLIC_API int createGroupFromModel(const std::string& modelLoc) {
    return UserContext::instance().createGroupFromModel(modelLoc);
}

LM_PUBLIC_API void primitive(Mat4 transform, const Json& prop) {
    auto t = transformNode(transform);
    if (prop.find("model") != prop.end()) {
        addChildFromModel(t, prop["model"]);
    }
    else {
        addChild(t, primitiveNode(prop));
    }
    addChild(rootNode(), t);
}

LM_NAMESPACE_END(LM_NAMESPACE)
