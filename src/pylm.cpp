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
    pybind11::class_<Component, Component_Py, Component::Ptr<Component>>(m, "Component")
        .def(pybind11::init<>())
        .def("key", &Component::key)
        .def("loc", &Component::loc)
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
        .PYLM_DEF_COMP_BIND(Component);

    auto sm = m.def_submodule("comp");
    sm.def("get", [](const std::string& locator) -> Component* {
        return comp::get<Component>(locator);
    }, pybind11::return_value_policy::reference);
    sm.def("load_plugin", &comp::load_plugin);
    sm.def("load_plugin_directory", &comp::load_plugin_directory);
    sm.def("unload_plugins", &comp::unload_plugins);
    sm.def("foreach_registered", &comp::foreach_registered);
}

// ------------------------------------------------------------------------------------------------

// Generate a function for the built-in interface
#define PYLM_DEF_ASSET_CREATE_FUNC(m, InterfaceType, interface_name) \
	m.def(fmt::format("load_{}", #interface_name).c_str(), \
		[](const std::string& name, const std::string& impl_key, const Json& prop) -> InterfaceType* { \
			auto* p = asset(name, fmt::format("{}::{}", #interface_name, impl_key), prop); \
			return dynamic_cast<InterfaceType*>(p); \
		})

// Bind user.h
static void bind_user(pybind11::module& m) {
    m.def("init", &init, "prop"_a = Json{});
    m.def("shutdown", &shutdown);
    m.def("info", &info);
    m.def("reset", &reset);
    m.def("asset", (Component*(*)(const std::string&, const std::string&, const Json&)) & asset, pybind11::return_value_policy::reference);
    m.def("asset", (std::string(*)(const std::string&)) & asset);
    m.def("build", &build);
    m.def("renderer", &renderer, pybind11::call_guard<pybind11::gil_scoped_release>());
    m.def("render", (void(*)(bool)) & render, "verbose"_a = true, pybind11::call_guard<pybind11::gil_scoped_release>());
    m.def("render", (void(*)(const std::string&, const Json&)) & render, pybind11::call_guard<pybind11::gil_scoped_release>());
    m.def("save", &save);
    m.def("buffer", &buffer);
    m.def("serialize", (void(*)(const std::string&)) & serialize);
    m.def("deserialize", (void(*)(const std::string&)) & deserialize);
    m.def("root_node", &root_node);
    m.def("primitive_node", &primitive_node);
    m.def("group_node", &group_node);
    m.def("instance_group_node", &instance_group_node);
    m.def("transform_node", &transform_node);
    m.def("add_child", &add_child);
    m.def("create_group_from_model", &create_group_from_model);
    m.def("primitive", &primitive);
	PYLM_DEF_ASSET_CREATE_FUNC(m, Mesh, mesh);
	PYLM_DEF_ASSET_CREATE_FUNC(m, Texture, texture);
	PYLM_DEF_ASSET_CREATE_FUNC(m, Material, material);
	PYLM_DEF_ASSET_CREATE_FUNC(m, Camera, camera);
	PYLM_DEF_ASSET_CREATE_FUNC(m, Medium, medium);
	PYLM_DEF_ASSET_CREATE_FUNC(m, Phase, phase);
	PYLM_DEF_ASSET_CREATE_FUNC(m, Film, film);
	PYLM_DEF_ASSET_CREATE_FUNC(m, Model, model);
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
    sm.def("init", &objloader::init);
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

// Bind debugio.h
static void bind_debugio(pybind11::module& m) {
    auto sm = m.def_submodule("debugio");
    sm.def("init", &debugio::init);
    sm.def("shutdown", &debugio::shutdown);
    sm.def("handle_message", &debugio::handle_message);
    sm.def("sync_user_context", &debugio::sync_user_context);
    sm.def("draw", &debugio::draw);
}

// ------------------------------------------------------------------------------------------------

// Bind debug.h
static void bind_debug(pybind11::module& m) {
    auto sm = m.def_submodule("debug");
    sm.def("poll_float", &debug::poll_float);
    sm.def("reg_on_poll_float", [](const debug::OnPollFloatFunc& onPollFloat) {
        // We must capture the callback function by value.
        // Otherwise the function would be dereferenced.
        debug::reg_on_poll_float([onPollFloat](const std::string& name, Float val) {
            pybind11::gil_scoped_acquire acquire;
            if (onPollFloat) {
                onPollFloat(name, val);
            }
        });
    });
    sm.def("attachToDebugger", &debug::attach_to_debugger);
}

// ------------------------------------------------------------------------------------------------

// Bind distributed.h
static void bind_distributed(pybind11::module& m) {
    auto sm = m.def_submodule("distributed");
    {
        auto sm2 = sm.def_submodule("master");
        sm2.def("init", &distributed::master::init);
        sm2.def("shutdown", &distributed::master::shutdown);
        sm2.def("print_worker_info", &distributed::master::print_worker_info);
        sm2.def("allow_worker_connection", &distributed::master::allow_worker_connection);
        sm2.def("sync", &distributed::master::sync, pybind11::call_guard<pybind11::gil_scoped_release>());
        sm2.def("gather_film", &distributed::master::gather_film, pybind11::call_guard<pybind11::gil_scoped_release>());
    }
    {
        auto sm2 = sm.def_submodule("worker");
        sm2.def("init", &distributed::worker::init);
        sm2.def("shutdown", &distributed::worker::shutdown);
        sm2.def("run", &distributed::worker::run, pybind11::call_guard<pybind11::gil_scoped_release>());
    }
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
        .def("aspect_ratio", &Film::aspect_ratio)
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
        .def_readwrite("wo", &PointGeometry::wo)
        .def_readwrite("t", &PointGeometry::t)
        .def_readwrite("u", &PointGeometry::u)
        .def_readwrite("v", &PointGeometry::v)
        .def_static("make_degenerated", &PointGeometry::make_degenerated)
        .def_static("make_infinite", &PointGeometry::make_infinite)
        .def_static("make_on_surface", (PointGeometry(*)(Vec3, Vec3, Vec2))&PointGeometry::make_on_surface)
        .def_static("make_on_surface", (PointGeometry(*)(Vec3, Vec3))&PointGeometry::make_on_surface)
        .def("opposite", &PointGeometry::opposite)
        .def("orthonormal_basis", &PointGeometry::orthonormal_basis);

    pybind11::class_<SceneInteraction>(m, "SceneInteraction")
        .def(pybind11::init<>())
        .def_readwrite("primitive", &SceneInteraction::primitive)
        .def_readwrite("geom", &SceneInteraction::geom)
        .def_readwrite("endpoint", &SceneInteraction::endpoint);

    auto sm = m.def_submodule("surface");
    sm.def("geometry_term", &surface::geometry_term);
}


// ------------------------------------------------------------------------------------------------

// Bind scene.h
static void bind_scene(pybind11::module& m) {
    pybind11::class_<RaySample>(m, "RaySample")
        .def_readwrite("sp", &RaySample::sp)
        .def_readwrite("wo", &RaySample::wo)
        .def_readwrite("weight", &RaySample::weight)
        .def("ray", &RaySample::ray);

    pybind11::enum_<SceneNodeType>(m, "SceneNodeType")
        .value("Primitive", SceneNodeType::Primitive)
        .value("Group", SceneNodeType::Group);

	class Scene_Py final : public Scene {
		virtual void construct(const Json& prop) override {
			PYBIND11_OVERLOAD(void, Scene, construct, prop);
		}
		virtual Component* load_asset(const std::string& name, const std::string& implKey, const Json& prop) override {
			PYBIND11_OVERLOAD_PURE(Component*, Scene, load_asset, name, implKey, prop);
		}
		virtual int root_node() override {
			PYBIND11_OVERLOAD_PURE(int, Scene, root_node);
		}
		virtual int create_node(SceneNodeType type, const Json& prop) override {
			PYBIND11_OVERLOAD_PURE(int, Scene, loadPrimitive, type, prop);
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
		virtual void build(const std::string& name, const Json& prop) override {
			PYBIND11_OVERLOAD_PURE(void, Scene, build, name, prop);
		}
		virtual std::optional<SceneInteraction> intersect(Ray ray, Float tmin, Float tmax) const override {
			PYBIND11_OVERLOAD_PURE(std::optional<SceneInteraction>, Scene, intersect, ray, tmin, tmax);
		}
		virtual bool is_light(const SceneInteraction& sp) const override {
			PYBIND11_OVERLOAD_PURE(bool, Scene, is_light, sp);
		}
		virtual bool is_specular(const SceneInteraction& sp, int comp) const override {
			PYBIND11_OVERLOAD_PURE(bool, Scene, is_specular, sp, comp);
		}
		virtual Ray primary_ray(Vec2 rp, Float aspect_ratio) const override {
			PYBIND11_OVERLOAD_PURE(Ray, Scene, primary_ray, rp, aspect_ratio);
		}
		virtual std::optional<Vec2> raster_position(Vec3 wo, Float aspect_ratio) const override {
			PYBIND11_OVERLOAD_PURE(std::optional<Vec2>, Scene, raster_position, wo, aspect_ratio);
		}
		virtual std::optional<RaySample> sample_ray(Rng& rng, const SceneInteraction& sp, Vec3 wi) const override {
			PYBIND11_OVERLOAD_PURE(std::optional<RaySample>, Scene, sample_ray, rng, sp, wi);
		}
		virtual std::optional<Vec3> sample_direction_given_comp(Rng& rng, const SceneInteraction& sp, int comp, Vec3 wi) const {
			PYBIND11_OVERLOAD_PURE(std::optional<Vec3>, Scene, sample_direction_given_comp, rng, sp, comp, wi);
		}
		virtual std::optional<RaySample> sample_direct_light(Rng& rng, const SceneInteraction& sp) const override {
			PYBIND11_OVERLOAD_PURE(std::optional<RaySample>, Scene, sample_direct_light, rng, sp);
		}
		virtual Float pdf(const SceneInteraction& sp, int comp, Vec3 wi, Vec3 wo) const override {
			PYBIND11_OVERLOAD_PURE(Float, Scene, pdf, sp, comp, wi, wo);
		}
		virtual Float pdf_comp(const SceneInteraction& sp, int comp, Vec3 wi) const override {
			PYBIND11_OVERLOAD_PURE(Float, Scene, pdf_comp, sp, comp, wi);
		}
		virtual Float pdf_direct_light(const SceneInteraction& sp, const SceneInteraction& spL, int compL, Vec3 wo) const override {
			PYBIND11_OVERLOAD_PURE(Float, Scene, pdf_direct_light, sp, spL, compL, wo);
		}
		virtual std::optional<DistanceSample> sample_distance(Rng& rng, const SceneInteraction& sp, Vec3 wo) const override {
			PYBIND11_OVERLOAD_PURE(std::optional<DistanceSample>, Scene, sample_distance, rng, sp, wo);
		}
		virtual Vec3 eval_transmittance(Rng& rng, const SceneInteraction& sp1, const SceneInteraction& sp2) const override {
			PYBIND11_OVERLOAD_PURE(Vec3, Scene, eval_transmittance, rng, sp1, sp2);
		}
		virtual Vec3 eval_contrb(const SceneInteraction& sp, int comp, Vec3 wi, Vec3 wo) const override {
			PYBIND11_OVERLOAD_PURE(Vec3, Scene, eval_contrb, sp, comp, wi, wo);
		}
		virtual Vec3 eval_contrb_endpoint(const SceneInteraction& sp, Vec3 wo) const override {
			PYBIND11_OVERLOAD_PURE(Vec3, Scene, eval_contrb_endpoint, sp, wo);
		}
		virtual std::optional<Vec3> reflectance(const SceneInteraction& sp, int comp) const override {
			PYBIND11_OVERLOAD_PURE(std::optional<Vec3>, Scene, reflectance, sp, comp);
		}
	};

    pybind11::class_<Scene, Scene_Py, Component, Component::Ptr<Scene>>(m, "Scene")
        .def(pybind11::init<>())
        .def("root_node", &Scene::root_node)
        .def("create_node", &Scene::create_node)
        .def("add_child", &Scene::add_child)
        .def("add_child_from_model", &Scene::add_child_from_model)
		.def("num_nodes", &Scene::num_nodes)
		.def("num_lights", &Scene::num_lights)
		.def("camera_node", &Scene::camera_node)
		.def("env_light_node", &Scene::env_light_node)
        .def("traverse_primitive_nodes", &Scene::traverse_primitive_nodes)
		.def("visit_node", &Scene::visit_node)
        .def("build", &Scene::build)
        .def("intersect", &Scene::intersect, "ray"_a = Ray{}, "tmin"_a = Eps, "tmax"_a = Inf)
        .def("is_light", &Scene::is_light)
        .def("is_specular", &Scene::is_specular)
        .def("primary_ray", &Scene::primary_ray)
        .def("sample_ray", &Scene::sample_ray)
        .def("eval_contrb_endpoint", &Scene::eval_contrb_endpoint)
        .def("reflectance", &Scene::reflectance)
        .PYLM_DEF_COMP_BIND(Scene);
}

// ------------------------------------------------------------------------------------------------

// Bind renderer.h
static void bind_renderer(pybind11::module& m) {
    class Renderer_Py final : public Renderer {
		PYLM_SERIALIZE_IMPL(Renderer);
        virtual void construct(const Json& prop) override {
            PYBIND11_OVERLOAD(void, Renderer, construct, prop);
        }
        virtual void render(const Scene* scene) const override {
            PYBIND11_OVERLOAD_PURE(void, Renderer, render, scene);
        }
    };
    pybind11::class_<Renderer, Renderer_Py, Component, Component::Ptr<Renderer>>(m, "Renderer")
        .def(pybind11::init<>())
        .def("render", &Renderer::render)
        .PYLM_DEF_COMP_BIND(Renderer);
}

// ------------------------------------------------------------------------------------------------

// Bind texture.h
static void bind_texture(pybind11::module& m) {
    pybind11::class_<TextureSize>(m, "TextureSize")
        .def_readwrite("w", &TextureSize::w)
        .def_readwrite("h", &TextureSize::h);
    class Texture_Py final : public Texture {
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

// Bind material.h
static void bind_material(pybind11::module& m) {
    pybind11::class_<MaterialDirectionSample>(m, "MaterialDirectionSample")
        .def(pybind11::init<>())
        .def_readwrite("wo", &MaterialDirectionSample::wo)
        .def_readwrite("comp", &MaterialDirectionSample::comp)
        .def_readwrite("weight", &MaterialDirectionSample::weight);

    class Material_Py final : public Material {
        virtual void construct(const Json& prop) override {
            PYBIND11_OVERLOAD(void, Material, construct, prop);
        }
        virtual bool is_specular(const PointGeometry& geom, int comp) const override {
            PYBIND11_OVERLOAD_PURE(bool, Material, is_specular, geom, comp);
        }
        virtual std::optional<MaterialDirectionSample> sample(Rng& rng, const PointGeometry& geom, Vec3 wi) const override {
            PYBIND11_OVERLOAD_PURE(std::optional<MaterialDirectionSample>, Material, sample, rng, geom, wi);
        }
        virtual std::optional<Vec3> reflectance(const PointGeometry& geom, int comp) const override {
            PYBIND11_OVERLOAD(std::optional<Vec3>, Material, reflectance, geom, comp);
        }
        virtual Float pdf(const PointGeometry& geom, int comp, Vec3 wi, Vec3 wo) const override {
            PYBIND11_OVERLOAD_PURE(Float, Material, pdf, geom, comp, wi, wo);
        }
        virtual Float pdf_comp(const PointGeometry& geom, int comp, Vec3 wi) const override {
            PYBIND11_OVERLOAD_PURE(Float, Material, pdf_comp, geom, comp, wi);
        }
        virtual Vec3 eval(const PointGeometry& geom, int comp, Vec3 wi, Vec3 wo) const override {
            PYBIND11_OVERLOAD_PURE(Vec3, Material, eval, geom, comp, wi, wo);
        }
    };
    pybind11::class_<Material, Material_Py, Component, Component::Ptr<Material>>(m, "Material")
        .def(pybind11::init<>())
        .def("is_specular", &Material::is_specular)
        .def("sample", &Material::sample)
        .def("reflectance", &Material::reflectance)
        .def("pdf", &Material::pdf)
        .def("eval", &Material::eval)
        .PYLM_DEF_COMP_BIND(Material);
}

// ------------------------------------------------------------------------------------------------

// Bind phase.h
static void bind_phase(pybind11::module& m) {
	pybind11::class_<PhaseDirectionSample>(m, "PhaseDirectionSample")
		.def(pybind11::init<>())
		.def_readwrite("wo", &PhaseDirectionSample::wo)
		.def_readwrite("weight", &PhaseDirectionSample::weight);

	class Phase_Py final : public Phase {
		virtual void construct(const Json& prop) override {
			PYBIND11_OVERLOAD(void, Phase, construct, prop);
		}
		virtual bool is_specular(const PointGeometry& geom) const override {
			PYBIND11_OVERLOAD_PURE(bool, Phase, is_specular, geom);
		}
		virtual std::optional<PhaseDirectionSample> sample(Rng& rng, const PointGeometry& geom, Vec3 wi) const override {
			PYBIND11_OVERLOAD_PURE(std::optional<PhaseDirectionSample>, Phase, sample, rng, geom, wi);
		}
		virtual Float pdf(const PointGeometry& geom, Vec3 wi, Vec3 wo) const override {
			PYBIND11_OVERLOAD_PURE(Float, Phase, pdf, geom, wi, wo);
		}
		virtual Vec3 eval(const PointGeometry& geom, Vec3 wi, Vec3 wo) const override {
			PYBIND11_OVERLOAD_PURE(Vec3, Phase, eval, geom, wi, wo);
		}
	};
	pybind11::class_<Phase, Phase_Py, Component, Component::Ptr<Phase>>(m, "Phase")
		.def(pybind11::init<>())
		.def("is_specular", &Phase::is_specular)
		.def("sample", &Phase::sample)
		.def("pdf", &Phase::pdf)
		.def("eval", &Phase::eval)
		.PYLM_DEF_COMP_BIND(Phase);
}

// ------------------------------------------------------------------------------------------------

// Bind medium.h
static void bind_medium(pybind11::module& m) {
	pybind11::class_<MediumDistanceSample>(m, "MediumDistanceSample")
		.def(pybind11::init<>())
		.def_readwrite("p", &MediumDistanceSample::p)
		.def_readwrite("weight", &MediumDistanceSample::weight)
		.def_readwrite("medium", &MediumDistanceSample::medium);

	class Medium_Py final : public Medium {
		virtual void construct(const Json& prop) override {
			PYBIND11_OVERLOAD(void, Medium, construct, prop);
		}
		virtual std::optional<MediumDistanceSample> sample_distance(Rng& rng, Ray ray, Float tmin, Float tmax) const override {
			PYBIND11_OVERLOAD_PURE(std::optional<MediumDistanceSample>, Medium, sample_distance, rng, ray, tmin, tmax);
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
		.def("phase", &Medium::phase, pybind11::return_value_policy::reference);
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
		.def("foreach_node", &Model::foreach_node);
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
	bind_distributed(m);
    bind_objloader(m);
    bind_progress(m);
    bind_debugio(m);
    bind_debug(m);
    bind_film(m);
    bind_surface(m);
    bind_scene(m);
    bind_renderer(m);
    bind_texture(m);
    bind_material(m);
	bind_medium(m);
	bind_phase(m);
	bind_model(m);
	bind_user(m);
}

LM_NAMESPACE_END(LM_NAMESPACE)
