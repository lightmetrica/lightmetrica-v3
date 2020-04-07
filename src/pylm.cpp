/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch_pylm.h>
#include <lm/lm.h>
#include <lm/pylm.h>

using namespace pybind11::literals;

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// ------------------------------------------------------------------------------------------------

// Bind common.h
static void bind_common(pybind11::module& m) {
    // Build config
    m.attr("Debug") = LM_CONFIG_DEBUG ? true : false;
    m.attr("Release") = LM_CONFIG_RELEASE ? true : false;
    m.attr("RelWithDebInfo") = LM_CONFIG_RELWITHDEBINFO ? true : false;

    // Supported floating point type
    enum class FloatType {
        Float32,
        Float64
    };
    pybind11::enum_<FloatType>(m, "FloatType")
        .value("Float32", FloatType::Float32)
        .value("Float64", FloatType::Float64)
        .export_values();
    m.attr("FloatT") = LM_SINGLE_PRECISION
        ? FloatType::Float32
        : FloatType::Float64;
}

// ------------------------------------------------------------------------------------------------

// Bind exception.h
static void bind_exception(pybind11::module& m) {
    pybind11::register_exception<lm::Exception>(m, "Exception");
}

// ------------------------------------------------------------------------------------------------

// Bind version.h
static void bind_version(pybind11::module& m) {
    auto sm = m.def_submodule("version");
    m.attr("major_version") = lm::version::major_version();
    m.attr("minor_version") = lm::version::minor_version();
    m.attr("patch_version") = lm::version::patch_version();
    m.attr("revision") = lm::version::revision();
    m.attr("build_timestamp") = lm::version::build_timestamp();
    m.attr("platform") = lm::version::platform();
    m.attr("architecture") = lm::version::architecture();
    m.attr("formatted") = lm::version::formatted();
}

// ------------------------------------------------------------------------------------------------

// Bind math.h
static void bind_math(pybind11::module& m) {
    // Constants
    m.attr("Inf") = Inf;
    m.attr("Eps") = Eps;
    m.attr("Pi") = Pi;

    // Ray
    pybind11::class_<Ray>(m, "Ray")
        .def(pybind11::init<>())
        .def("__init__", [](Ray& r, Vec3 o, Vec3 d) {
            r.o = o;
            r.d = d;
        })
        .def_readwrite("o", &Ray::o)
        .def_readwrite("d", &Ray::d);

    // Rng
    pybind11::class_<Rng>(m, "Rng")
        .def(pybind11::init<>())
        .def(pybind11::init<int>())
        .def("u", &Rng::u);

    // Helper functions
    m.def("identity", []() -> Mat4 {
        return Mat4(1_f);
    });
    m.def("rotate", [](Float angle, Vec3 axis) -> Mat4 {
        return glm::rotate(angle, axis);
    });
    m.def("translate", [](Vec3 v) -> Mat4 {
        return glm::translate(v);
    });
    m.def("scale", [](Vec3 s) -> Mat4 {
        return glm::scale(s);
    });
    m.def("rad", &glm::radians<Float>);
    m.def("v3", [](Float v1, Float v2, Float v3) -> Vec3 {
        return Vec3(v1, v2, v3);
    });

    // math namespace
    auto sm = m.def_submodule("math");
    sm.def("orthonormal_basis", &math::orthonormal_basis);
    sm.def("safe_sqrt", &math::safe_sqrt);
    sm.def("sq", &math::sq);
    sm.def("reflection", &math::reflection);
    pybind11::class_<math::RefractionResult>(sm, "RefractionResult")
        .def_readwrite("wt", &math::RefractionResult::wt)
        .def_readwrite("total", &math::RefractionResult::total);
    sm.def("refraction", &math::refraction);
    sm.def("sample_cosine_weighted", &math::sample_cosine_weighted);
    sm.def("balance_heuristic", &math::balance_heuristic);
}

// ------------------------------------------------------------------------------------------------

// Bind component.h
static void bind_component(pybind11::module& m) {
    class Component_Py final : public Component {
    public:
        PYLM_SERIALIZE_IMPL(Component);
        virtual void construct(const Json& prop) override {
            PYBIND11_OVERLOAD(void, Component, construct, prop);
        }
    };
    using make_loc_func_ptr = const std::string(Component::*)(const std::string&) const;
    pybind11::class_<Component, Component_Py, Component::Ptr<Component>>(m, "Component")
        .def(pybind11::init<>())
        .def("key", &Component::key)
        .def("loc", &Component::loc)
        .def("make_loc", (make_loc_func_ptr)&Component::make_loc)
        .def("parentLoc", &Component::parent_loc)
        .def("construct", &Component::construct)
        .def("underlying", &Component::underlying, pybind11::return_value_policy::reference)
        .def("underlying_value", &Component::underlying_value, "query"_a = "")
        .def("save", [](Component* self) -> pybind11::bytes {
            std::ostringstream os;
            {
                OutputArchive ar(os);
                self->save(ar);
            }
            return pybind11::bytes(os.str());
        })
        .def("load", [](Component* self, pybind11::bytes data) {
            std::istringstream is(data);
            InputArchive ar(is);
            self->load(ar);
        })
        .def("save_to_file", [](Component* self, const std::string& path) {
            std::ofstream os(path, std::ios::out | std::ios::binary);
            serial::save_comp_owned(os, self, self->loc());
        })
        .PYLM_DEF_COMP_BIND(Component);

    auto sm = m.def_submodule("comp");
    sm.def("get", [](const std::string& locator) -> Component* {
        return comp::get<Component>(locator);
    }, pybind11::return_value_policy::reference);
    sm.def("load_plugin", &comp::load_plugin);
    sm.def("load_plugin_directory", &comp::load_plugin_directory);
    sm.def("unload_plugin", &comp::unload_plugin);
    sm.def("unload_all_plugins", &comp::unload_all_plugins);
    sm.def("foreach_registered", &comp::foreach_registered);
}

// ------------------------------------------------------------------------------------------------

// Generate a function for the built-in interface
#define PYLM_DEF_ASSET_CREATE_AND_GET_FUNC(m, InterfaceType, interface_name) \
    m.def(fmt::format("load_{}", #interface_name).c_str(), \
        [](const std::string& name, const std::string& impl_key, const Json& prop) -> InterfaceType* { \
            auto* p = assets()->load_asset(name, fmt::format("{}::{}", #interface_name, impl_key), prop); \
            return dynamic_cast<InterfaceType*>(p); \
        }, pybind11::return_value_policy::reference); \
    m.def(fmt::format("get_{}", #interface_name).c_str(), \
        [](const std::string& loc) -> InterfaceType* { \
            return comp::get<InterfaceType>(loc); \
        }, pybind11::return_value_policy::reference)

// Bind user.h
static void bind_user(pybind11::module& m) {
    m.def("init", &init, "prop"_a = Json{});
    m.def("shutdown", &shutdown);
    m.def("reset", &reset);
    m.def("info", &info);
    m.def("assets", &assets, pybind11::return_value_policy::reference);
    m.def("save_state_to_file", &save_state_to_file);
    m.def("load_state_from_file", &load_state_from_file);

    // Expose some functions in comp namespace to lm namespace
    m.def("load", [](const std::string& name, const std::string& impl_key, const Json& prop) -> Component* {
        return assets()->load_asset(name, impl_key, prop);
    }, pybind11::return_value_policy::reference);
    m.def("load_serialized", [](const std::string& name, const std::string& path) -> Component* {
        return assets()->load_serialized(name, path);
    }, pybind11::return_value_policy::reference);
    m.def("get", [](const std::string& loc) -> Component* {
        return comp::detail::get(loc);
    }, pybind11::return_value_policy::reference);
    m.def("load_plugin", &comp::load_plugin);

    // Bind load and get functions for each asset interface
    PYLM_DEF_ASSET_CREATE_AND_GET_FUNC(m, Mesh, mesh);
    PYLM_DEF_ASSET_CREATE_AND_GET_FUNC(m, Texture, texture);
    PYLM_DEF_ASSET_CREATE_AND_GET_FUNC(m, Material, material);
    PYLM_DEF_ASSET_CREATE_AND_GET_FUNC(m, Camera, camera);
    PYLM_DEF_ASSET_CREATE_AND_GET_FUNC(m, Light, light);
    PYLM_DEF_ASSET_CREATE_AND_GET_FUNC(m, Medium, medium);
    PYLM_DEF_ASSET_CREATE_AND_GET_FUNC(m, Phase, phase);
    PYLM_DEF_ASSET_CREATE_AND_GET_FUNC(m, Volume, volume);
    PYLM_DEF_ASSET_CREATE_AND_GET_FUNC(m, Film, film);
    PYLM_DEF_ASSET_CREATE_AND_GET_FUNC(m, Model, model);
    PYLM_DEF_ASSET_CREATE_AND_GET_FUNC(m, AssetGroup, asset_group);
    PYLM_DEF_ASSET_CREATE_AND_GET_FUNC(m, Accel, accel);
    PYLM_DEF_ASSET_CREATE_AND_GET_FUNC(m, Scene, scene);
    PYLM_DEF_ASSET_CREATE_AND_GET_FUNC(m, Renderer, renderer);
}

// ------------------------------------------------------------------------------------------------

#define PYLM_DEF_ASSET_CREATE_MEMBER_FUNC(InterfaceType, interface_name) \
    def(fmt::format("load_{}", #interface_name).c_str(), \
        [](AssetGroup& self, const std::string& name, const std::string& impl_key, const Json& prop) -> InterfaceType* { \
            auto* p = self.load_asset(name, fmt::format("{}::{}", #interface_name, impl_key), prop); \
            return dynamic_cast<InterfaceType*>(p); \
        }, pybind11::return_value_policy::reference)

// Bind assetgroup.h
static void bind_asset_group(pybind11::module& m) {
    class AssetGroup_Py final : public AssetGroup {
        virtual void construct(const Json& prop) override {
            PYBIND11_OVERLOAD(void, AssetGroup, construct, prop);
        }
        virtual Component* load_asset(const std::string& name, const std::string& impl_key, const Json& prop) override {
            PYBIND11_OVERLOAD_PURE(Component*, AssetGroup, load_asset, name, impl_key, prop);
        }
        virtual Component* load_serialized(const std::string& name, const std::string& path) override {
            PYBIND11_OVERLOAD_PURE(Component*, AssetGroup, load_serialized, name, path);
        }
    };
    pybind11::class_<AssetGroup, AssetGroup_Py, Component, Component::Ptr<AssetGroup>>(m, "AssetGroup")
        .def(pybind11::init<>())
        .def("load_asset", &AssetGroup::load_asset, pybind11::return_value_policy::reference)
        .def("load_serialized", &AssetGroup::load_serialized, pybind11::return_value_policy::reference)
        .PYLM_DEF_ASSET_CREATE_MEMBER_FUNC(Mesh, mesh)
        .PYLM_DEF_ASSET_CREATE_MEMBER_FUNC(Texture, texture)
        .PYLM_DEF_ASSET_CREATE_MEMBER_FUNC(Material, material)
        .PYLM_DEF_ASSET_CREATE_MEMBER_FUNC(Camera, camera)
        .PYLM_DEF_ASSET_CREATE_MEMBER_FUNC(Light, light)
        .PYLM_DEF_ASSET_CREATE_MEMBER_FUNC(Medium, medium)
        .PYLM_DEF_ASSET_CREATE_MEMBER_FUNC(Phase, phase)
        .PYLM_DEF_ASSET_CREATE_MEMBER_FUNC(Volume, volume)
        .PYLM_DEF_ASSET_CREATE_MEMBER_FUNC(Film, film)
        .PYLM_DEF_ASSET_CREATE_MEMBER_FUNC(Model, model)
        .PYLM_DEF_ASSET_CREATE_MEMBER_FUNC(AssetGroup, asset_group)
        .PYLM_DEF_ASSET_CREATE_MEMBER_FUNC(Accel, accel)
        .PYLM_DEF_ASSET_CREATE_MEMBER_FUNC(Scene, scene)
        .PYLM_DEF_ASSET_CREATE_MEMBER_FUNC(Renderer, renderer);
}

// ------------------------------------------------------------------------------------------------

// Bind logger.h
static void bind_logger(pybind11::module& m) {
    auto sm = m.def_submodule("log");

    pybind11::enum_<log::LogLevel>(sm, "LogLevel")
        .value("Debug", log::LogLevel::Debug)
        .value("Info", log::LogLevel::Info)
        .value("Warn", log::LogLevel::Warn)
        .value("Err", log::LogLevel::Err)
        .value("Progress", log::LogLevel::Progress)
        .value("ProgressEnd", log::LogLevel::ProgressEnd);

    sm.def("init", &log::init, "type"_a = log::DefaultType, "prop"_a = Json{});
    sm.def("shutdown", &log::shutdown);
    using logFuncPtr = void(*)(log::LogLevel, int, const char*, int, const char*);
    sm.def("log", (logFuncPtr)&log::log);
    sm.def("update_indentation", &log::update_indentation);
    using setSeverityFuncPtr = void(*)(int);
    sm.def("set_severity", (setSeverityFuncPtr)&log::set_severity);

    using LoggerContext = log::LoggerContext;
    class LoggerContext_Py final : public LoggerContext {
        virtual void construct(const Json& prop) override {
            pybind11::gil_scoped_acquire acquire;
            PYBIND11_OVERLOAD(void, LoggerContext, construct, prop);
        }
        virtual void log(log::LogLevel level, int severity, const char* filename, int line, const char* message) override {
            pybind11::gil_scoped_acquire acquire;
            PYBIND11_OVERLOAD_PURE(void, LoggerContext, log, level, severity, filename, line, message);
        }
        virtual void update_indentation(int n) override {
            pybind11::gil_scoped_acquire acquire;
            PYBIND11_OVERLOAD_PURE(void, LoggerContext, update_indentation, n);
        }
        virtual void set_severity(int severity) override {
            pybind11::gil_scoped_acquire acquire;
            PYBIND11_OVERLOAD_PURE(void, LoggerContext, set_severity, severity);
        }
    };
    pybind11::class_<LoggerContext, LoggerContext_Py, Component, Component::Ptr<LoggerContext>>(sm, "LoggerContext")
        .def(pybind11::init<>())
        .def("log", &LoggerContext::log)
        .def("update_indentation", &LoggerContext::update_indentation)
        .PYLM_DEF_COMP_BIND(LoggerContext);
}

// ------------------------------------------------------------------------------------------------

// Bind parallel.h
static void bind_parallel(pybind11::module& m) {    
    auto sm = m.def_submodule("parallel");
    sm.def("init", &parallel::init, "type"_a = parallel::DefaultType, "prop"_a = Json{});
    sm.def("shutdown", &parallel::shutdown);
    sm.def("num_threads", &parallel::num_threads);
    sm.def("foreach", [](long long numSamples, const parallel::ParallelProcessFunc& processFunc) {
        // Release GIL and let the C++ to create new threads
        pybind11::gil_scoped_release release;
        parallel::foreach(numSamples, [&](long long index, int threadId) {
            // Reacquire GIL when we call Python function
            pybind11::gil_scoped_acquire acquire;
            processFunc(index, threadId);
        });
    });
}

// ------------------------------------------------------------------------------------------------

// Bind objloader.h
static void bind_objloader(pybind11::module& m) {
    auto sm = m.def_submodule("objloader");
    sm.def("init", &objloader::init, "type"_a = objloader::DefaultType, "prop"_a = Json{});
    sm.def("shutdown", &objloader::shutdown);
}

// ------------------------------------------------------------------------------------------------

// Bind progress.h
static void bind_progress(pybind11::module& m) {
    auto sm = m.def_submodule("progress");

    pybind11::enum_<progress::ProgressMode>(sm, "ProgressMode")
        .value("Samples", progress::ProgressMode::Samples)
        .value("Time", progress::ProgressMode::Time);

    sm.def("init", &progress::init, "type"_a = progress::DefaultType, "prop"_a = Json{});
    sm.def("shutdown", &progress::shutdown);
    sm.def("start", &progress::start);
    sm.def("end", &progress::end);
    sm.def("update", &progress::update);

    using ProgressContext = progress::ProgressContext;
    class ProgressContext_Py final : public ProgressContext {
        virtual void construct(const Json& prop) override {
            pybind11::gil_scoped_acquire acquire;
            PYBIND11_OVERLOAD(void, ProgressContext, construct, prop);
        }
        virtual void start(progress::ProgressMode mode, long long total, double totalTime) override {
            pybind11::gil_scoped_acquire acquire;
            PYBIND11_OVERLOAD_PURE(void, ProgressContext, start, mode, total, totalTime);
        }
        virtual void end() override {
            pybind11::gil_scoped_acquire acquire;
            PYBIND11_OVERLOAD_PURE(void, ProgressContext, end);
        }
        virtual void update(long long processed) override {
            pybind11::gil_scoped_acquire acquire;
            PYBIND11_OVERLOAD_PURE(void, ProgressContext, update, processed);
        }
        virtual void update_time(Float elapsed) override {
            pybind11::gil_scoped_acquire acquire;
            PYBIND11_OVERLOAD_PURE(void, ProgressContext, update_time, elapsed);
        }
    };
    pybind11::class_<ProgressContext, ProgressContext_Py, Component, Component::Ptr<ProgressContext>>(sm, "ProgressContext")
        .def(pybind11::init<>())
        .def("start", &ProgressContext::start)
        .def("end", &ProgressContext::end)
        .def("update", &ProgressContext::update)
        .def("update_time", &ProgressContext::update_time)
        .PYLM_DEF_COMP_BIND(ProgressContext);
}

// ------------------------------------------------------------------------------------------------

// Bind debug.h
static void bind_debug(pybind11::module& m) {
    auto sm = m.def_submodule("debug");
    sm.def("poll", &debug::poll);
    sm.def("reg_on_poll", [](const debug::OnPollFunc& func) {
        // We must capture the callback function by value.
        // Otherwise the function would be dereferenced.
        debug::reg_on_poll([func](const Json& val) {
            pybind11::gil_scoped_acquire acquire;
            if (func) {
                func(val);
            }
        });
    });
    sm.def("attach_to_debugger", &debug::attach_to_debugger);
    sm.def("print_asset_tree", &debug::print_asset_tree, "visualize_weak_refs"_a = false);
}

// ------------------------------------------------------------------------------------------------

// Bind film.h
static void bind_film(pybind11::module& m) {
    // Film size
    pybind11::class_<FilmSize>(m, "FilmSize")
        .def_readwrite("w", &FilmSize::w)
        .def_readwrite("h", &FilmSize::h);

    // Film buffer
    pybind11::class_<FilmBuffer>(m, "FilmBuffer", pybind11::buffer_protocol())
        // Register buffer description
        .def_buffer([](FilmBuffer& buf) -> pybind11::buffer_info {
            return pybind11::buffer_info(
                buf.data,
                sizeof(Float),
                pybind11::format_descriptor<Float>::format(),
                3,
                { buf.h, buf.w, 3 },
                { 3 * buf.w * sizeof(Float),
                  3 * sizeof(Float),
                  sizeof(Float) }
            );
        });
    
    // Film
    class Film_Py final : public Film {
    public:
        virtual void construct(const Json& prop) override {
            PYBIND11_OVERLOAD(void, Film, construct, prop);
        }
        virtual FilmSize size() const override {
            PYBIND11_OVERLOAD_PURE(FilmSize, Film, size);
        }
        virtual long long num_pixels() const override {
            PYBIND11_OVERLOAD_PURE(long long, Film, num_pixels);
        }
        virtual void set_pixel(int x, int y, Vec3 v) override {
            PYBIND11_OVERLOAD_PURE(void, Film, set_pixel, x, y, v);
        }
        virtual bool save(const std::string& outpath) const override {
            PYBIND11_OVERLOAD_PURE(bool, Film, save, outpath);
        }
        virtual FilmBuffer buffer() override {
            PYBIND11_OVERLOAD_PURE(FilmBuffer, Film, buffer);
        }
        virtual void accum(const Film* film) override {
            PYBIND11_OVERLOAD_PURE(void, Film, accum, film);
        }
        virtual void splat_pixel(int x, int y, Vec3 v) override {
            PYBIND11_OVERLOAD_PURE(void, Film, splat_pixel, x, y, v);
        }
        virtual void update_pixel(int x, int y, const PixelUpdateFunc& updateFunc) override {
            PYBIND11_OVERLOAD_PURE(void, Film, update_pixel, x, y, updateFunc);
        }
        virtual void rescale(Float s) override {
            PYBIND11_OVERLOAD_PURE(void, Film, rescale, s);
        }
        virtual void clear() override {
            PYBIND11_OVERLOAD_PURE(void, Film, clear);
        }
    };
    pybind11::class_<Film, Film_Py, Component, Component::Ptr<Film>>(m, "Film")
        .def(pybind11::init<>())
        .def("loc", &Film::loc)
        .def("size", &Film::size)
        .def("num_pixels", &Film::num_pixels)
        .def("set_pixel", &Film::set_pixel)
        .def("save", &Film::save)
        .def("aspect", &Film::aspect)
        .def("buffer", &Film::buffer)
        .PYLM_DEF_COMP_BIND(Film);
}

// ------------------------------------------------------------------------------------------------

// Bind surface.h
static void bind_surface(pybind11::module& m) {
    pybind11::class_<PointGeometry>(m, "PointGeometry")
        .def(pybind11::init<>())
        .def_readwrite("degenerated", &PointGeometry::degenerated)
        .def_readwrite("infinite", &PointGeometry::infinite)
        .def_readwrite("p", &PointGeometry::p)
        .def_readwrite("n", &PointGeometry::n)
        .def_readwrite("gn", &PointGeometry::gn)
        .def_readwrite("wo", &PointGeometry::wo)
        .def_readwrite("t", &PointGeometry::t)
        .def_readwrite("u", &PointGeometry::u)
        .def_readwrite("v", &PointGeometry::v)
        .def_static("make_degenerated", &PointGeometry::make_degenerated)
        .def_static("make_infinite", (PointGeometry(*)(Vec3)) & PointGeometry::make_infinite)
        .def_static("make_infinite", (PointGeometry(*)(Vec3, Vec3))&PointGeometry::make_infinite)
        .def_static("make_on_surface", (PointGeometry(*)(Vec3, Vec3, Vec3, Vec2))&PointGeometry::make_on_surface)
        .def_static("make_on_surface", (PointGeometry(*)(Vec3, Vec3, Vec3))&PointGeometry::make_on_surface)
        .def("opposite", &PointGeometry::opposite)
        .def("orthonormal_basis_twosided", &PointGeometry::orthonormal_basis_twosided);

    pybind11::class_<SceneInteraction>(m, "SceneInteraction")
        .def(pybind11::init<>())
        .def_readonly("type", &SceneInteraction::type)
        .def_readwrite("primitive", &SceneInteraction::primitive)
        .def_readwrite("geom", &SceneInteraction::geom);

    pybind11::enum_<SceneInteraction::Type>(m, "SceneInteractionType")
        .value("None", SceneInteraction::None)
        .value("CameraEndpoint", SceneInteraction::CameraEndpoint)
        .value("LightEndpoint", SceneInteraction::LightEndpoint)
        .value("SurfaceInteraction", SceneInteraction::SurfaceInteraction)
        .value("MediumInteraction", SceneInteraction::MediumInteraction)
        .value("Endpoint", SceneInteraction::Endpoint)
        .value("Midpoint", SceneInteraction::Midpoint)
        .export_values();

    auto sm = m.def_submodule("surface");
    sm.def("geometry_term", &surface::geometry_term);
}

// ------------------------------------------------------------------------------------------------

// Bind scenenode.h
static void bind_scenenode(pybind11::module& m) {
    pybind11::enum_<SceneNodeType>(m, "SceneNodeType")
        .value("Primitive", SceneNodeType::Primitive)
        .value("Group", SceneNodeType::Group);

    pybind11::class_<SceneNode::Primitive>(m, "SceneNode_Primitive")
        .def_readonly("mesh", &SceneNode::Primitive::mesh)
        .def_readonly("material", &SceneNode::Primitive::material)
        .def_readonly("light", &SceneNode::Primitive::light)
        .def_readonly("camera", &SceneNode::Primitive::camera)
        .def_readonly("medium", &SceneNode::Primitive::medium);

    pybind11::class_<SceneNode::Group>(m, "SceneNode_Group")
        .def_readonly("children", &SceneNode::Group::children)
        .def_readonly("instanced", &SceneNode::Group::instanced)
        .def_readonly("local_transform", &SceneNode::Group::local_transform);

    pybind11::class_<SceneNode>(m, "SceneNode")
        .def_readonly("type", &SceneNode::type)
        .def_readonly("index", &SceneNode::index)
        .def_readonly("primitive", &SceneNode::primitive)
        .def_readonly("group", &SceneNode::group);
}

// ------------------------------------------------------------------------------------------------

// Bind scene.h
static void bind_scene(pybind11::module& m) {
    class Scene_Py final : public Scene {
        virtual void construct(const Json& prop) override {
            PYBIND11_OVERLOAD(void, Scene, construct, prop);
        }
        // ----------------------------------------------------------------------------------------
        virtual void reset() override {
            PYBIND11_OVERLOAD_PURE(void, Scene, reset);
        }
        // ----------------------------------------------------------------------------------------
        virtual int root_node() override {
            PYBIND11_OVERLOAD_PURE(int, Scene, root_node);
        }
        virtual int create_primitive_node(const Json& prop) override {
            PYBIND11_OVERLOAD_PURE(int, Scene, create_primitive_node, prop);
        }
        virtual int create_group_node(Mat4 transform) override {
            PYBIND11_OVERLOAD_PURE(int, Scene, create_group_node, transform);
        }
        virtual int create_instance_group_node() override {
            PYBIND11_OVERLOAD_PURE(int, Scene, create_instance_group_node);
        }
        virtual void add_child(int parent, int child) override {
            PYBIND11_OVERLOAD_PURE(void, Scene, add_child, parent, child);
        }
        virtual void add_child_from_model(int parent, const std::string& modelLoc) override {
            PYBIND11_OVERLOAD_PURE(void, Scene, add_child_from_model, parent, modelLoc);
        }
        virtual int create_group_from_model(const std::string& modelLoc) override {
            PYBIND11_OVERLOAD_PURE(int, Scene, create_group_from_model, modelLoc);
        }
        virtual int num_nodes() const override {
            PYBIND11_OVERLOAD_PURE(int, Scene, num_nodes);
        }
        virtual int num_lights() const override {
            PYBIND11_OVERLOAD_PURE(int, Scene, num_lights);
        }
        virtual int camera_node() const override {
            PYBIND11_OVERLOAD_PURE(int, Scene, camera_node);
        }
        virtual int medium_node() const override {
            PYBIND11_OVERLOAD_PURE(int, Scene, medium_node);
        }
        virtual int env_light_node() const override {
            PYBIND11_OVERLOAD_PURE(int, Scene, env_light_node);
        }
        virtual void traverse_primitive_nodes(const NodeTraverseFunc& traverseFunc) const override {
            PYBIND11_OVERLOAD_PURE(void, Scene, traverse_primitive_nodes, traverseFunc);
        }
        virtual void visit_node(int node_index, const VisitNodeFunc& visit) const override {
            PYBIND11_OVERLOAD_PURE(void, Scene, visit_node, node_index, visit);
        }
        virtual const SceneNode& node_at(int node_index) const override {
            PYBIND11_OVERLOAD_PURE(const SceneNode&, Scene, node_at, node_index);
        }
        // ----------------------------------------------------------------------------------------
        virtual Accel* accel() const override {
            PYBIND11_OVERLOAD_PURE(Accel*, Scene, accel);
        }
        virtual void set_accel(const std::string& accel_loc) override {
            PYBIND11_OVERLOAD_PURE(void, Scene, set_accel, accel_loc);
        }
        virtual void build() override {
            PYBIND11_OVERLOAD_PURE(void, Scene, build);
        }
        virtual std::optional<SceneInteraction> intersect(Ray ray, Float tmin, Float tmax) const override {
            PYBIND11_OVERLOAD_PURE(std::optional<SceneInteraction>, Scene, intersect, ray, tmin, tmax);
        }
        // ----------------------------------------------------------------------------------------
        virtual bool is_light(const SceneInteraction& sp) const override {
            PYBIND11_OVERLOAD_PURE(bool, Scene, is_light, sp);
        }
        virtual bool is_camera(const SceneInteraction& sp) const override {
            PYBIND11_OVERLOAD_PURE(bool, Scene, is_camera, sp);
        }
        // ----------------------------------------------------------------------------------------
        virtual LightSelectionSample sample_light_selection(Float u) const override {
            PYBIND11_OVERLOAD_PURE(LightSelectionSample, Scene, sample_light_selection, u);
        }
        virtual Float pdf_light_selection(int light_index) const override {
            PYBIND11_OVERLOAD_PURE(Float, Scene, pdf_light_selection, light_index);
        }
        virtual LightPrimitiveIndex light_primitive_index_at(int light_index) const override {
            PYBIND11_OVERLOAD_PURE(LightPrimitiveIndex, Scene, light_primitive_index_at, light_index);
        }
        virtual int light_index_at(int node_index) const override {
            PYBIND11_OVERLOAD_PURE(int, Scene, light_index_at, node_index);
        }
    };

    pybind11::class_<Scene, Scene_Py, Component, Component::Ptr<Scene>>(m, "Scene")
        .def(pybind11::init<>())
        //
        .def("reset", &Scene::reset)
        //
        .def("root_node", &Scene::root_node)
        .def("create_primitive_node", &Scene::create_primitive_node)
        .def("create_group_node", &Scene::create_group_node)
        .def("create_instance_group_node", &Scene::create_instance_group_node)
        .def("add_child", &Scene::add_child)
        .def("add_child_from_model", &Scene::add_child_from_model)
        .def("create_group_from_model", &Scene::create_group_from_model)
        .def("add_primitive", &Scene::add_primitive)
        .def("add_transformed_primitive", &Scene::add_transformed_primitive)
        .def("traverse_primitive_nodes", &Scene::traverse_primitive_nodes)
        .def("visit_node", &Scene::visit_node)
        .def("node_at", &Scene::node_at, pybind11::return_value_policy::reference)
        .def("num_nodes", &Scene::num_nodes)
        .def("num_lights", &Scene::num_lights)
        .def("camera_node", &Scene::camera_node)
        .def("medium_node", &Scene::medium_node)
        .def("env_light_node", &Scene::env_light_node)
        .def("camera", &Scene::camera, pybind11::return_value_policy::reference)
        //
        .def("require_primitive", &Scene::require_primitive)
        .def("require_camera", &Scene::require_camera)
        .def("require_light", &Scene::require_light)
        .def("require_accel", &Scene::require_accel)
        .def("require_renderable", &Scene::require_renderable)
        //
        .def("accel", &Scene::accel, pybind11::return_value_policy::reference)
        .def("set_accel", &Scene::set_accel)
        .def("build", &Scene::build)
        .def("intersect", &Scene::intersect, "ray"_a = Ray{}, "tmin"_a = Eps, "tmax"_a = Inf)
        .def("visible", &Scene::visible)
        //
        .def("is_light", &Scene::is_light)
        .def("is_camera", &Scene::is_camera)
        //
        .def("sample_light_selection", &Scene::sample_light_selection)
        .def("pdf_light_selection", &Scene::pdf_light_selection)
        .def("light_primitive_index_at", &Scene::light_primitive_index_at)
        .def("light_index_at", &Scene::light_index_at)
        .PYLM_DEF_COMP_BIND(Scene);
}

// ------------------------------------------------------------------------------------------------

// Bind accel.h
static void bind_accel(pybind11::module& m) {
    pybind11::class_<Accel::Hit>(m, "Accel_Hit")
        .def_readwrite("t", &Accel::Hit::t)
        .def_readwrite("uv", &Accel::Hit::uv)
        .def_readwrite("global_transform", &Accel::Hit::global_transform)
        .def_readwrite("primitive", &Accel::Hit::primitive)
        .def_readwrite("face", &Accel::Hit::face);

    class Accel_Py final : public Accel {
        virtual void construct(const Json& prop) override {
            PYBIND11_OVERLOAD(void, Accel, construct, prop);
        }
        virtual void build(const Scene& scene) override {
            PYBIND11_OVERLOAD_PURE(void, Accel, build, scene);
        }
        virtual std::optional<Hit> intersect(Ray ray, Float tmin, Float tmax) const override {
            PYBIND11_OVERLOAD_PURE(std::optional<Hit>, Accel, intersect, ray, tmin, tmax);
        }
    };
    pybind11::class_<Accel, Accel_Py, Component, Component::Ptr<Accel>>(m, "Accel")
        .def(pybind11::init<>())
        .def("build", &Accel::build)
        .def("intersect", &Accel::intersect)
        .PYLM_DEF_COMP_BIND(Accel);
}

// ------------------------------------------------------------------------------------------------

// Bind renderer.h
static void bind_renderer(pybind11::module& m) {
    class Renderer_Py final : public Renderer {
        PYLM_SERIALIZE_IMPL(Renderer);
        virtual void construct(const Json& prop) override {
            PYBIND11_OVERLOAD(void, Renderer, construct, prop);
        }
        virtual Json render() const override {
            PYBIND11_OVERLOAD_PURE(Json, Renderer, render);
        }
    };
    pybind11::class_<Renderer, Renderer_Py, Component, Component::Ptr<Renderer>>(m, "Renderer")
        .def(pybind11::init<>())
        .def("render", &Renderer::render, pybind11::call_guard<pybind11::gil_scoped_release>())
        .PYLM_DEF_COMP_BIND(Renderer);
}

// ------------------------------------------------------------------------------------------------

// Bind texture.h
static void bind_texture(pybind11::module& m) {
    pybind11::class_<TextureSize>(m, "TextureSize")
        .def_readwrite("w", &TextureSize::w)
        .def_readwrite("h", &TextureSize::h);
    class Texture_Py final : public Texture {
        virtual void construct(const Json& prop) override {
            PYBIND11_OVERLOAD(void, Texture, construct, prop);
        }
        virtual TextureSize size() const override {
            PYBIND11_OVERLOAD_PURE(TextureSize, Texture, size);
        }
        virtual Vec3 eval(Vec2 t) const override {
            PYBIND11_OVERLOAD_PURE(Vec3, Texture, eval, t);
        }
        virtual Vec3 eval_by_pixel_coords(int x, int y) const override {
            PYBIND11_OVERLOAD_PURE(Vec3, Texture, eval_by_pixel_coords, x, y);
        }
    };
    pybind11::class_<Texture, Texture_Py, Component, Component::Ptr<Texture>>(m, "Texture")
        .def(pybind11::init<>())
        .def("size", &Texture::size)
        .def("eval", &Texture::eval)
        .def("eval_by_pixel_coords", &Texture::eval_by_pixel_coords)
        .PYLM_DEF_COMP_BIND(Texture);
}

// ------------------------------------------------------------------------------------------------

// Bind mesh.h
static void bind_mesh(pybind11::module& m) {
    pybind11::class_<Mesh::Point>(m, "Mesh_Point")
        .def_readwrite("p", &Mesh::Point::p)
        .def_readwrite("n", &Mesh::Point::n)
        .def_readwrite("t", &Mesh::Point::t);

    pybind11::class_<Mesh::InterpolatedPoint>(m, "Mesh_InterpolatedPoint")
        .def_readwrite("p", &Mesh::InterpolatedPoint::p)
        .def_readwrite("n", &Mesh::InterpolatedPoint::n)
        .def_readwrite("gn", &Mesh::InterpolatedPoint::gn)
        .def_readwrite("t", &Mesh::InterpolatedPoint::t);

    pybind11::class_<Mesh::Tri>(m, "Mesh_Tri")
        .def_readwrite("p1", &Mesh::Tri::p1)
        .def_readwrite("p2", &Mesh::Tri::p2)
        .def_readwrite("p3", &Mesh::Tri::p3);

    class Mesh_Py final : public Mesh {
        virtual void construct(const Json& prop) override {
            PYBIND11_OVERLOAD(void, Mesh, construct, prop);
        }
        virtual void foreach_triangle(const ProcessTriangleFunc& process_triangle) const override {
            PYBIND11_OVERLOAD_PURE(void, Mesh, foreach_triangle, process_triangle);
        }
        virtual Tri triangle_at(int face) const override {
            PYBIND11_OVERLOAD_PURE(Tri, Mesh, triangle_at, face);
        }
        virtual InterpolatedPoint surface_point(int face, Vec2 uv) const override {
            PYBIND11_OVERLOAD_PURE(InterpolatedPoint, Mesh, surface_point, face, uv);
        }
        virtual int num_triangles() const override {
            PYBIND11_OVERLOAD_PURE(int, Mesh, num_triangles);
        }
    };
    pybind11::class_<Mesh, Mesh_Py, Component, Component::Ptr<Mesh>>(m, "Mesh")
        .def(pybind11::init<>())
        .def("foreach_triangle", &Mesh::foreach_triangle)
        .def("triangle_at", &Mesh::triangle_at)
        .def("surface_point", &Mesh::surface_point)
        .def("num_triangles", &Mesh::num_triangles)
        .PYLM_DEF_COMP_BIND(Mesh);
}

// ------------------------------------------------------------------------------------------------

// Bind material.h
static void bind_material(pybind11::module& m) {
    pybind11::class_<Material::DirectionSample>(m, "Material_DirectionSample")
        .def(pybind11::init<>())
        .def_readwrite("wo", &Material::DirectionSample::wo)
        .def_readwrite("weight", &Material::DirectionSample::weight);

    class Material_Py final : public Material {
        virtual void construct(const Json& prop) override {
            PYBIND11_OVERLOAD(void, Material, construct, prop);
        }
        virtual ComponentSample sample_component(const ComponentSampleU& u, const PointGeometry& geom) const override {
            PYBIND11_OVERLOAD_PURE(ComponentSample, Material, sample_component, u, geom);
        }
        virtual Float pdf_component(int comp, const PointGeometry& geom) const override {
            PYBIND11_OVERLOAD_PURE(Float, Material, pdf_component, comp, geom);
        }
        virtual std::optional<DirectionSample> sample_direction(const DirectionSampleU& u, const PointGeometry& geom, Vec3 wi, int comp, TransDir trans_dir) const override {
            PYBIND11_OVERLOAD_PURE(std::optional<DirectionSample>, Material, sample_direction, u, geom, wi, comp, trans_dir);
        }
        virtual Float pdf_direction(const PointGeometry& geom, Vec3 wi, Vec3 wo, int comp, bool eval_delta) const override {
            PYBIND11_OVERLOAD_PURE(Float, Material, pdf_direction, geom, wi, wo, comp, eval_delta);
        }
        virtual Vec3 eval(const PointGeometry& geom, Vec3 wi, Vec3 wo, int comp, TransDir trans_dir, bool eval_delta) const override {
            PYBIND11_OVERLOAD_PURE(Vec3, Material, eval, geom, wi, wo, comp, trans_dir, eval_delta);
        }
        virtual bool is_specular_component(int comp) const override {
            PYBIND11_OVERLOAD_PURE(bool, Material, is_specular_component, comp);
        }
        virtual Vec3 reflectance(const PointGeometry& geom) const override {
            PYBIND11_OVERLOAD_PURE(Vec3, Material, reflectance, geom);
        }
    };
    pybind11::class_<Material, Material_Py, Component, Component::Ptr<Material>>(m, "Material")
        .def(pybind11::init<>())
        .def("sample_direction", &Material::sample_direction)
        .def("pdf_direction", &Material::pdf_direction)
        .def("eval", &Material::eval)
        .def("is_specular_component", &Material::is_specular_component)
        .def("reflectance", &Material::reflectance)
        .PYLM_DEF_COMP_BIND(Material);
}

// ------------------------------------------------------------------------------------------------

// Bind phase.h
static void bind_phase(pybind11::module& m) {
    pybind11::class_<Phase::DirectionSample>(m, "Phase_DirectionSample")
        .def(pybind11::init<>())
        .def_readwrite("wo", &Phase::DirectionSample::wo)
        .def_readwrite("weight", &Phase::DirectionSample::weight);

    class Phase_Py final : public Phase {
        virtual void construct(const Json& prop) override {
            PYBIND11_OVERLOAD(void, Phase, construct, prop);
        }
        virtual std::optional<DirectionSample> sample_direction(const DirectionSampleU& u, const PointGeometry& geom, Vec3 wi) const override {
            PYBIND11_OVERLOAD_PURE(std::optional<DirectionSample>, Phase, sample_direction, u, geom, wi);
        }
        virtual Float pdf_direction(const PointGeometry& geom, Vec3 wi, Vec3 wo) const override {
            PYBIND11_OVERLOAD_PURE(Float, Phase, pdf_direction, geom, wi, wo);
        }
        virtual Vec3 eval(const PointGeometry& geom, Vec3 wi, Vec3 wo) const override {
            PYBIND11_OVERLOAD_PURE(Vec3, Phase, eval, geom, wi, wo);
        }
    };
    pybind11::class_<Phase, Phase_Py, Component, Component::Ptr<Phase>>(m, "Phase")
        .def(pybind11::init<>())
        .def("sample_direction", &Phase::sample_direction)
        .def("pdf_direction", &Phase::pdf_direction)
        .def("eval", &Phase::eval)
        .PYLM_DEF_COMP_BIND(Phase);
}

// ------------------------------------------------------------------------------------------------

// Bind medium.h
static void bind_medium(pybind11::module& m) {
    pybind11::class_<Medium::DistanceSample>(m, "Medium_DistanceSample")
        .def(pybind11::init<>())
        .def_readwrite("p", &Medium::DistanceSample::p)
        .def_readwrite("weight", &Medium::DistanceSample::weight)
        .def_readwrite("medium", &Medium::DistanceSample::medium);

    class Medium_Py final : public Medium {
        virtual void construct(const Json& prop) override {
            PYBIND11_OVERLOAD(void, Medium, construct, prop);
        }
        virtual std::optional<DistanceSample> sample_distance(Rng& rng, Ray ray, Float tmin, Float tmax) const override {
            PYBIND11_OVERLOAD_PURE(std::optional<DistanceSample>, Medium, sample_distance, rng, ray, tmin, tmax);
        }
        virtual Vec3 eval_transmittance(Rng& rng, Ray ray, Float tmin, Float tmax) const override {
            PYBIND11_OVERLOAD_PURE(Vec3, Medium, eval_transmittance, rng, ray, tmin, tmax);
        }
        virtual bool is_emitter() const override {
            PYBIND11_OVERLOAD_PURE(bool, Medium, is_emitter);
        }
        virtual const Phase* phase() const override {
            PYBIND11_OVERLOAD_PURE(const Phase*, Medium, phase);
        }
    };
    pybind11::class_<Medium, Medium_Py, Component, Component::Ptr<Medium>>(m, "Medium")
        .def(pybind11::init<>())
        .def("sample_distance", &Medium::sample_distance)
        .def("eval_transmittance", &Medium::eval_transmittance)
        .def("is_emitter", &Medium::is_emitter)
        .def("phase", &Medium::phase, pybind11::return_value_policy::reference)
        .PYLM_DEF_COMP_BIND(Medium);
}

// Bind volume.h
static void bind_volume(pybind11::module& m) {
    class Volume_Py final : public Volume {
        virtual void construct(const Json& prop) override {
            PYBIND11_OVERLOAD(void, Volume, construct, prop);
        }
        virtual Bound bound() const override {
            PYBIND11_OVERLOAD_PURE(Bound, Volume, bound);
        }
        virtual bool has_scalar() const override {
            PYBIND11_OVERLOAD_PURE(bool, Volume, has_scalar);
        }
        virtual Float max_scalar() const override {
            PYBIND11_OVERLOAD_PURE(Float, Volume, has_scalar);
        }
        virtual Float eval_scalar(Vec3 p) const override {
            PYBIND11_OVERLOAD(Float, Volume, eval_scalar, p);
        }
        virtual bool has_color() const override {
            PYBIND11_OVERLOAD_PURE(bool, Volume, has_color);
        }
        virtual Vec3 eval_color(Vec3 p) const override {
            PYBIND11_OVERLOAD(Vec3, Volume, eval_color, p);
        }
    };
    pybind11::class_<Volume, Volume_Py, Component, Component::Ptr<Volume>>(m, "Volume")
        .def(pybind11::init<>())
        .def("bound", &Volume::bound)
        .def("has_scalar", &Volume::has_scalar)
        .def("max_scalar", &Volume::max_scalar)
        .def("eval_scalar", &Volume::eval_scalar)
        .def("has_color", &Volume::has_color)
        .def("eval_color", &Volume::eval_color)
        .PYLM_DEF_COMP_BIND(Volume);
}

// ------------------------------------------------------------------------------------------------

// Bind model.h
static void bind_model(pybind11::module& m) {
    class Model_Py final : public Model {
        virtual void construct(const Json& prop) override {
            PYBIND11_OVERLOAD(void, Model, construct, prop);
        }
        virtual void create_primitives(const CreatePrimitiveFunc& create_primitive) const override {
            PYBIND11_OVERLOAD_PURE(void, Model, create_primitives, create_primitive)
        }
        virtual void foreach_node(const VisitNodeFuncType& visit) const override {
            PYBIND11_OVERLOAD_PURE(void, Model, foreach_node, visit);
        }
    };
    pybind11::class_<Model, Model_Py, Component, Component::Ptr<Model>>(m, "Model")
        .def(pybind11::init<>())
        .def("create_primitives", &Model::create_primitives)
        .def("foreach_node", &Model::foreach_node)
        .PYLM_DEF_COMP_BIND(Model);
}

// ------------------------------------------------------------------------------------------------

// Bind camera.h
static void bind_camera(pybind11::module& m) {
    pybind11::class_<Camera::RaySample>(m, "Camera_RaySample")
        .def(pybind11::init<>())
        .def_readonly("geom", &Camera::RaySample::geom)
        .def_readonly("wo", &Camera::RaySample::wo)
        .def_readonly("weight", &Camera::RaySample::weight);

    pybind11::class_<Camera::DirectionSample>(m, "Camera_DirectionSample")
        .def(pybind11::init<>())
        .def_readonly("wo", &Camera::DirectionSample::wo)
        .def_readonly("weight", &Camera::DirectionSample::weight);

    pybind11::class_<Camera::PositionSample>(m, "Camera_PositionSample")
        .def(pybind11::init<>())
        .def_readonly("geom", &Camera::PositionSample::geom)
        .def_readonly("weight", &Camera::PositionSample::weight);

    class Camera_Py final : public Camera {
        virtual void construct(const Json& prop) override {
            PYBIND11_OVERLOAD(void, Camera, construct, prop);
        }
        // ----------------------------------------------------------------------------------------
        virtual void set_aspect_ratio(Float aspect) override {
            PYBIND11_OVERLOAD_PURE(void, Camera, set_aspect_ratio, aspect);
        }
        // ----------------------------------------------------------------------------------------
        virtual Mat4 view_matrix() const override {
            PYBIND11_OVERLOAD_PURE(Mat4, Camera, view_matrix);
        }
        virtual Mat4 projection_matrix() const override {
            PYBIND11_OVERLOAD_PURE(Mat4, Camera, projection_matrix);
        }
        // ----------------------------------------------------------------------------------------
        virtual Ray primary_ray(Vec2 rp) const override {
            PYBIND11_OVERLOAD_PURE(Ray, Camera, primary_ray, rp);
        }
        virtual std::optional<RaySample> sample_ray(const RaySampleU& u) const override {
            PYBIND11_OVERLOAD_PURE(std::optional<RaySample>, Camera, sample_ray, u);
        }
        virtual Float pdf_ray(const PointGeometry& geom, Vec3 wo) const override {
            PYBIND11_OVERLOAD_PURE(Float, Camera, pdf_ray, geom, wo);
        }
        // ----------------------------------------------------------------------------------------
        virtual std::optional<DirectionSample> sample_direction(const DirectionSampleU& u, const PointGeometry& geom) const override {
            PYBIND11_OVERLOAD_PURE(std::optional<DirectionSample>, Camera, sample_direction, u, geom);
        }
        virtual Float pdf_direction(const PointGeometry& geom, Vec3 wo) const override {
            PYBIND11_OVERLOAD_PURE(Float, Camera, pdf_direction, geom, wo);
        }
        // ----------------------------------------------------------------------------------------
        virtual std::optional<PositionSample> sample_position(const PositionSampleU& u) const override {
            PYBIND11_OVERLOAD_PURE(std::optional<PositionSample>, Camera, sample_position, u);
        }
        virtual Float pdf_position(const PointGeometry& geom) const override {
            PYBIND11_OVERLOAD_PURE(Float, Camera, pdf_position, geom);
        }
        // ----------------------------------------------------------------------------------------
        virtual std::optional<RaySample> sample_direct(const RaySampleU& u, const PointGeometry& geom) const override {
            PYBIND11_OVERLOAD_PURE(std::optional<RaySample>, Camera, sample_direct, u, geom);
        }
        virtual Float pdf_direct(const PointGeometry& geom, const PointGeometry& geomE, Vec3 wo) const override {
            PYBIND11_OVERLOAD_PURE(Float, Camera, pdf_direct, geom, geomE, wo);
        }
        // ----------------------------------------------------------------------------------------
        virtual std::optional<Vec2> raster_position(Vec3 wo) const override {
            PYBIND11_OVERLOAD_PURE(std::optional<Vec2>, Camera, raster_position, wo);
        }
        virtual bool is_connectable(const PointGeometry& geom) const override {
            PYBIND11_OVERLOAD_PURE(bool, Camera, is_connectable, geom);
        }
        virtual Vec3 eval(Vec3 wo) const override {
            PYBIND11_OVERLOAD_PURE(Vec3, Camera, eval, wo);
        }
    };
    pybind11::class_<Camera, Camera_Py, Component, Component::Ptr<Camera>>(m, "Camera")
        .def(pybind11::init<>())
        //
        .def("set_aspect_ratio", &Camera::set_aspect_ratio)
        //
        .def("view_matrix", &Camera::view_matrix)
        .def("projection_matrix", &Camera::projection_matrix)
        //
        .def("primary_ray", &Camera::primary_ray)
        .def("sample_ray", &Camera::sample_ray)
        .def("sample_direction", &Camera::sample_direction)
        .def("pdf_direction", &Camera::pdf_direction)
        .def("sample_position", &Camera::sample_position)
        .def("pdf_position", &Camera::pdf_position)
        //
        .def("sample_direct", &Camera::sample_direct)
        .def("pdf_direct", &Camera::pdf_direct)
        //
        .def("raster_position", &Camera::raster_position)
        .def("is_connectable", &Camera::is_connectable)
        .def("eval", &Camera::eval)
        .PYLM_DEF_COMP_BIND(Camera);
}

// ------------------------------------------------------------------------------------------------

// Bind light.h
static void bind_light(pybind11::module& m) {
    pybind11::class_<Light::RaySample>(m, "Light_RaySample")
        .def(pybind11::init<>())
        .def_readwrite("geom", &Light::RaySample::geom)
        .def_readwrite("wo", &Light::RaySample::wo)
        .def_readwrite("weight", &Light::RaySample::weight);

    class Light_Py final : public Light {
        virtual void construct(const Json& prop) override {
            PYBIND11_OVERLOAD(void, Light, construct, prop);
        }
        // ----------------------------------------------------------------------------------------
        virtual std::optional<RaySample> sample_ray(const RaySampleU& u, const Transform& transform) const override {
            PYBIND11_OVERLOAD_PURE(std::optional<RaySample>, Light, sample_ray, u, transform);
        }
        virtual Float pdf_ray(const PointGeometry& geom, Vec3 wo, const Transform& transform, bool eval_delta) const override {
            PYBIND11_OVERLOAD_PURE(Float, Light, pdf_ray, geom, wo, transform, eval_delta);
        }
        // ----------------------------------------------------------------------------------------
        virtual std::optional<DirectionSample> sample_direction(const DirectionSampleU& u, const PointGeometry& geom) const override {
            PYBIND11_OVERLOAD_PURE(std::optional<DirectionSample>, Light, sample_direction, u, geom);
        }
        virtual Float pdf_direction(const PointGeometry& geom, Vec3 wo) const override {
            PYBIND11_OVERLOAD_PURE(Float, Light, pdf_direction, geom, wo);
        }
        // ----------------------------------------------------------------------------------------
        virtual std::optional<PositionSample> sample_position(const PositionSampleU& u, const Transform& transform) const override {
            PYBIND11_OVERLOAD_PURE(std::optional<PositionSample>, Light, sample_position, u, transform);
        }
        virtual Float pdf_position(const PointGeometry& geom, const Transform& transform) const override {
            PYBIND11_OVERLOAD_PURE(Float, Light, pdf_position, geom, transform);
        }
        // ----------------------------------------------------------------------------------------
        virtual std::optional<RaySample> sample_direct(const RaySampleU& u, const PointGeometry& geom, const Transform& transform) const override {
            PYBIND11_OVERLOAD_PURE(std::optional<RaySample>, Light, sample_direct, u, geom, transform);
        }
        virtual Float pdf_direct(const PointGeometry& geom, const PointGeometry& geomL, const Transform& transform, Vec3 wo, bool eval_delta) const override {
            PYBIND11_OVERLOAD_PURE(Float, Light, pdf_direct, geom, geomL, transform, wo, eval_delta);
        }
        // ---------------------------------------------------------------------------------------
        virtual bool is_specular() const override {
            PYBIND11_OVERLOAD_PURE(bool, Light, is_specular);
        }
        virtual bool is_infinite() const override {
            PYBIND11_OVERLOAD_PURE(bool, Light, is_infinite);
        }
        virtual bool is_connectable(const PointGeometry& geom) const override {
            PYBIND11_OVERLOAD_PURE(bool, Light, is_connectable, geom);
        }
        virtual Vec3 eval(const PointGeometry& geom, Vec3 wo, bool eval_delta) const override {
            PYBIND11_OVERLOAD_PURE(Vec3, Light, eval, geom, wo, eval_delta);
        }
    };
    pybind11::class_<Light, Light_Py, Component, Component::Ptr<Light>>(m, "Light")
        .def(pybind11::init<>())
        .def("is_infinite", &Light::is_infinite)
        //
        .def("sample_ray", &Light::sample_ray)
        .def("sample_direction", &Light::sample_direction)
        .def("pdf_direction", &Light::pdf_direction)
        .def("sample_position", &Light::sample_position)
        .def("pdf_position", &Light::pdf_position)
        //
        .def("sample_direct", &Light::sample_direct)
        .def("pdf_direct", &Light::pdf_direct)
        //
        .def("is_connectable", &Light::is_connectable)
        .def("eval", &Light::eval)
        .PYLM_DEF_COMP_BIND(Light);
}

// ------------------------------------------------------------------------------------------------

// Bind light.h
static void bind_volume(pybind11::module& m) {

    class Volume_Py final : public Volume {
        virtual void construct(const Json& prop) override {
            PYBIND11_OVERLOAD(void, Volume, construct, prop);
        }
        virtual Bound bound() const override {
            PYBIND11_OVERLOAD_PURE(Bound, Volume, bound);
        }
        virtual bool has_scalar() const override {
            PYBIND11_OVERLOAD_PURE(bool, Volume, has_scalar);
        }
        virtual Float max_scalar() const override {
            PYBIND11_OVERLOAD_PURE(Float, Volume, has_scalar);
        }
        virtual Float eval_scalar(Vec3 p) const override {
            PYBIND11_OVERLOAD_PURE(Float, Volume, eval_scalar, p);
        }
        virtual bool has_color() const override {
            PYBIND11_OVERLOAD_PURE(bool, Volume, has_color);
        }
        virtual Vec3 eval_color(Vec3 p) const override {
            PYBIND11_OVERLOAD_PURE(Vec3, Volume, eval_color, p);
        }
    };
    pybind11::class_<Volume, Volume_Py, Component, Component::Ptr<Volume>>(m, "Volume")
        .def(pybind11::init<>())
        .def("bound", &Volume::bound)
        .def("has_scalar", &Volume::has_scalar)
        .def("max_scalar", &Volume::max_scalar)
        .def("eval_scalar", &Volume::eval_scalar)
        .def("has_color", &Volume::has_color)
        .def("eval_color", &Volume::eval_color)
        .PYLM_DEF_COMP_BIND(Volume);
}

// ------------------------------------------------------------------------------------------------

PYBIND11_MODULE(pylm, m) {
    m.doc() = R"x(
        pylm: Python binding of Lightmetrica.
    )x";

    // Bind headers.
    // Changing binding order might cause runtime error.
    bind_common(m);
    bind_exception(m);
    bind_version(m);
    bind_math(m);
    bind_component(m);
    bind_logger(m);
    bind_parallel(m);
    bind_objloader(m);
    bind_progress(m);
    bind_debug(m);
    bind_film(m);
    bind_surface(m);
    bind_scenenode(m);
    bind_scene(m);
    bind_accel(m);
    bind_renderer(m);
    bind_texture(m);
    bind_mesh(m);
    bind_material(m);
    bind_medium(m);
    bind_volume(m);
    bind_phase(m);
    bind_model(m);
    bind_camera(m);
    bind_light(m);
    bind_volume(m);
    bind_asset_group(m);
    bind_user(m);
}

LM_NAMESPACE_END(LM_NAMESPACE)
