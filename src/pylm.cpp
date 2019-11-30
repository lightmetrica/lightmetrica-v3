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
    // Debug or Release mode
    m.attr("Debug") = LM_DEBUG_MODE ? true : false;

    // Configuration
    enum class ConfigType {
        Debug,
        Release,
        RelWithDebInfo
    };
    pybind11::enum_<ConfigType>(m, "ConfigType")
        .value("Debug", ConfigType::Debug)
        .value("Release", ConfigType::Release)
        .value("RelWithDebInfo", ConfigType::RelWithDebInfo);
    m.attr("BuildConfig") =
        LM_CONFIG_DEBUG ? ConfigType::Debug :
        LM_CONFIG_RELEASE ? ConfigType::Release :
        ConfigType::RelWithDebInfo;

    // Supported floating point type
    enum class FloatPrecisionType {
        Float32,
        Float64
    };
    pybind11::enum_<FloatPrecisionType>(m, "FloatPrecisionType")
        .value("Float32", FloatPrecisionType::Float32)
        .value("Float64", FloatPrecisionType::Float64)
        .export_values();
    m.attr("FloatPrecision") = LM_SINGLE_PRECISION
        ? FloatPrecisionType::Float32
        : FloatPrecisionType::Float64;
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
    m.attr("majorVersion") = lm::version::majorVersion();
    m.attr("minorVersion") = lm::version::minorVersion();
    m.attr("patchVersion") = lm::version::patchVersion();
    m.attr("revision") = lm::version::revision();
    m.attr("buildTimestamp") = lm::version::buildTimestamp();
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
    sm.def("orthonormalBasis", &math::orthonormalBasis);
    sm.def("safeSqrt", &math::safeSqrt);
    sm.def("sq", &math::sq);
    sm.def("reflection", &math::reflection);
    pybind11::class_<math::RefractionResult>(sm, "RefractionResult")
        .def_readwrite("wt", &math::RefractionResult::wt)
        .def_readwrite("total", &math::RefractionResult::total);
    sm.def("refraction", &math::refraction);
    sm.def("sampleCosineWeighted", &math::sampleCosineWeighted);
    sm.def("balanceHeuristic", &math::balanceHeuristic);
}

// ------------------------------------------------------------------------------------------------

// Bind component.h
static void bind_component(pybind11::module& m) {
    class Component_Py final : public Component {
    public:
        PYLM_SERIALIZE_IMPL(Component)
        virtual void construct(const Json& prop) override {
            PYBIND11_OVERLOAD(void, Component, construct, prop);
        }
    };
    pybind11::class_<Component, Component_Py, Component::Ptr<Component>>(m, "Component")
        .def(pybind11::init<>())
        .def("key", &Component::key)
        .def("loc", &Component::loc)
        .def("parentLoc", &Component::parentLoc)
        .def("construct", &Component::construct)
        .def("underlying", &Component::underlying, pybind11::return_value_policy::reference)
        .def("underlyingValue", &Component::underlyingValue, "query"_a = "")
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
    sm.def("loadPlugin", &comp::loadPlugin);
    sm.def("loadPluginDirectory", &comp::loadPluginDirectory);
    sm.def("unloadPlugins", &comp::unloadPlugins);
    sm.def("foreachRegistered", &comp::foreachRegistered);
}

// ------------------------------------------------------------------------------------------------

// Bind user.h
static void bind_user(pybind11::module& m) {
    m.def("init", &init, "prop"_a = Json{});
    m.def("shutdown", &shutdown);
    m.def("info", &info);
    m.def("reset", &reset);
    m.def("asset", (std::string(*)(const std::string&, const std::string&, const Json&)) & asset);
    m.def("asset", (std::string(*)(const std::string&)) & asset);
    m.def("build", &build);
    m.def("renderer", &renderer, pybind11::call_guard<pybind11::gil_scoped_release>());
    m.def("render", (void(*)(bool)) & render, "verbose"_a = true, pybind11::call_guard<pybind11::gil_scoped_release>());
    m.def("render", (void(*)(const std::string&, const Json&)) & render, pybind11::call_guard<pybind11::gil_scoped_release>());
    m.def("save", &save);
    m.def("buffer", &buffer);
    m.def("serialize", (void(*)(const std::string&)) & serialize);
    m.def("deserialize", (void(*)(const std::string&)) & deserialize);
    m.def("rootNode", &rootNode);
    m.def("primitiveNode", &primitiveNode);
    m.def("groupNode", &groupNode);
    m.def("instanceGroupNode", &instanceGroupNode);
    m.def("transformNode", &transformNode);
    m.def("addChild", &addChild);
    m.def("createGroupFromModel", &createGroupFromModel);
    m.def("primitive", &primitive);
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
    sm.def("updateIndentation", &log::updateIndentation);
    using setSeverityFuncPtr = void(*)(int);
    sm.def("setSeverity", (setSeverityFuncPtr)&log::setSeverity);

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
        virtual void updateIndentation(int n) override {
            pybind11::gil_scoped_acquire acquire;
            PYBIND11_OVERLOAD_PURE(void, LoggerContext, updateIndentation, n);
        }
        virtual void setSeverity(int severity) override {
            pybind11::gil_scoped_acquire acquire;
            PYBIND11_OVERLOAD_PURE(void, LoggerContext, setSeverity, severity);
        }
    };
    pybind11::class_<LoggerContext, LoggerContext_Py, Component::Ptr<LoggerContext>>(sm, "LoggerContext")
        .def(pybind11::init<>())
        .def("log", &LoggerContext::log)
        .def("updateIndentation", &LoggerContext::updateIndentation)
        .PYLM_DEF_COMP_BIND(LoggerContext);
}

// ------------------------------------------------------------------------------------------------

// Bind parallel.h
static void bind_parallel(pybind11::module& m) {    
    auto sm = m.def_submodule("parallel");
    sm.def("init", &parallel::init, "type"_a = parallel::DefaultType, "prop"_a = Json{});
    sm.def("shutdown", &parallel::shutdown);
    sm.def("numThreads", &parallel::numThreads);
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
        virtual void updateTime(Float elapsed) override {
            pybind11::gil_scoped_acquire acquire;
            PYBIND11_OVERLOAD_PURE(void, ProgressContext, updateTime, elapsed);
        }
    };
    pybind11::class_<ProgressContext, ProgressContext_Py, Component::Ptr<ProgressContext>>(sm, "ProgressContext")
        .def(pybind11::init<>())
        .def("start", &ProgressContext::start)
        .def("end", &ProgressContext::end)
        .def("update", &ProgressContext::update)
        .def("updateTime", &ProgressContext::updateTime)
        .PYLM_DEF_COMP_BIND(ProgressContext);
}

// ------------------------------------------------------------------------------------------------

// Bind debugio.h
static void bind_debugio(pybind11::module& m) {
    auto sm = m.def_submodule("debugio");
    sm.def("init", &debugio::init);
    sm.def("shutdown", &debugio::shutdown);
    sm.def("handleMessage", &debugio::handleMessage);
    sm.def("syncUserContext", &debugio::syncUserContext);
    sm.def("draw", &debugio::draw);
}

// ------------------------------------------------------------------------------------------------

// Bind debug.h
static void bind_debug(pybind11::module& m) {
    auto sm = m.def_submodule("debug");
    sm.def("pollFloat", &debug::pollFloat);
    sm.def("regOnPollFloat", [](const debug::OnPollFloatFunc& onPollFloat) {
        // We must capture the callback function by value.
        // Otherwise the function would be dereferenced.
        debug::regOnPollFloat([onPollFloat](const std::string& name, Float val) {
            pybind11::gil_scoped_acquire acquire;
            if (onPollFloat) {
                onPollFloat(name, val);
            }
        });
    });
    sm.def("attachToDebugger", &debug::attachToDebugger);
}

// ------------------------------------------------------------------------------------------------

// Bind distributed.h
static void bind_distributed(pybind11::module& m) {
    auto sm = m.def_submodule("distributed");
    {
        auto sm2 = sm.def_submodule("master");
        sm2.def("init", &distributed::master::init);
        sm2.def("shutdown", &distributed::master::shutdown);
        sm2.def("printWorkerInfo", &distributed::master::printWorkerInfo);
        sm2.def("allowWorkerConnection", &distributed::master::allowWorkerConnection);
        sm2.def("sync", &distributed::master::sync, pybind11::call_guard<pybind11::gil_scoped_release>());
        sm2.def("gatherFilm", &distributed::master::gatherFilm, pybind11::call_guard<pybind11::gil_scoped_release>());
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
        virtual long long numPixels() const override {
            PYBIND11_OVERLOAD_PURE(long long, Film, numPixels);
        }
        virtual void setPixel(int x, int y, Vec3 v) override {
            PYBIND11_OVERLOAD_PURE(void, Film, setPixel, x, y, v);
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
        virtual void splatPixel(int x, int y, Vec3 v) override {
            PYBIND11_OVERLOAD_PURE(void, Film, splatPixel, x, y, v);
        }
        virtual void updatePixel(int x, int y, const PixelUpdateFunc& updateFunc) override {
            PYBIND11_OVERLOAD_PURE(void, Film, updatePixel, x, y, updateFunc);
        }
        virtual void rescale(Float s) override {
            PYBIND11_OVERLOAD_PURE(void, Film, rescale, s);
        }
        virtual void clear() override {
            PYBIND11_OVERLOAD_PURE(void, Film, clear);
        }
    };
    pybind11::class_<Film, Film_Py, Component::Ptr<Film>>(m, "Film")
        .def(pybind11::init<>())
        .def("loc", &Film::loc)
        .def("size", &Film::size)
        .def("numPixels", &Film::numPixels)
        .def("setPixel", &Film::setPixel)
        .def("save", &Film::save)
        .def("aspectRatio", &Film::aspectRatio)
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
        .def_static("makeDegenerated", &PointGeometry::makeDegenerated)
        .def_static("makeInfinite", &PointGeometry::makeInfinite)
        .def_static("makeOnSurface", (PointGeometry(*)(Vec3, Vec3, Vec2))&PointGeometry::makeOnSurface)
        .def_static("makeOnSurface", (PointGeometry(*)(Vec3, Vec3))&PointGeometry::makeOnSurface)
        .def("opposite", &PointGeometry::opposite)
        .def("orthonormalBasis", &PointGeometry::orthonormalBasis);

    pybind11::class_<SceneInteraction>(m, "SceneInteraction")
        .def(pybind11::init<>())
        .def_readwrite("primitive", &SceneInteraction::primitive)
        .def_readwrite("geom", &SceneInteraction::geom)
        .def_readwrite("endpoint", &SceneInteraction::endpoint);

    auto sm = m.def_submodule("surface");
    sm.def("geometryTerm", &surface::geometryTerm);
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
        virtual bool renderable() const override {
            PYBIND11_OVERLOAD_PURE(bool, Scene, renderable);
        }
        virtual std::optional<std::string> loadAsset(const std::string& name, const std::string& implKey, const Json& prop) override {
            PYBIND11_OVERLOAD_PURE(std::optional<std::string>, Scene, loadAsset, name, implKey, prop);
        }
        virtual int rootNode() override {
            PYBIND11_OVERLOAD_PURE(int, Scene, rootNode);
        }
        virtual int createNode(SceneNodeType type, const Json& prop) override {
            PYBIND11_OVERLOAD_PURE(int, Scene, loadPrimitive, type, prop);
        }
        virtual void addChild(int parent, int child) override {
            PYBIND11_OVERLOAD_PURE(void, Scene, addChild, parent, child);
        }
        virtual void addChildFromModel(int parent, const std::string& modelLoc) override {
            PYBIND11_OVERLOAD_PURE(void, Scene, addChildFromModel, parent, modelLoc);
        }
		virtual int createGroupFromModel(const std::string& modelLoc) override {
			PYBIND11_OVERLOAD_PURE(int, Scene, createGroupFromModel, modelLoc);
		}
        virtual int envLightNode() const override {
            PYBIND11_OVERLOAD_PURE(int, Scene, envLightNode);
        }
        virtual int numLights() const override {
            PYBIND11_OVERLOAD_PURE(int, Scene, numLights);
        }
        virtual void traversePrimitiveNodes(const NodeTraverseFunc& traverseFunc) const override {
            PYBIND11_OVERLOAD_PURE(void, Scene, traversePrimitiveNodes, traverseFunc);
        }
        virtual void visitNode(int nodeIndex, const VisitNodeFunc& visit) const override {
            PYBIND11_OVERLOAD_PURE(void, Scene, visitNode, nodeIndex, visit);
        }
        virtual const SceneNode& nodeAt(int nodeIndex) const override {
            PYBIND11_OVERLOAD_PURE(const SceneNode&, Scene, nodeAt, nodeIndex);
        }
        virtual void build(const std::string& name, const Json& prop) override {
            PYBIND11_OVERLOAD_PURE(void, Scene, build, name, prop);
        }
        virtual std::optional<SceneInteraction> intersect(Ray ray, Float tmin, Float tmax) const override {
            PYBIND11_OVERLOAD_PURE(std::optional<SceneInteraction>, Scene, intersect, ray, tmin, tmax);
        }
        virtual bool isLight(const SceneInteraction& sp) const override {
            PYBIND11_OVERLOAD_PURE(bool, Scene, isLight, sp);
        }
        virtual bool isSpecular(const SceneInteraction& sp, int comp) const override {
            PYBIND11_OVERLOAD_PURE(bool, Scene, isSpecular, sp, comp);
        }
        virtual Ray primaryRay(Vec2 rp, Float aspectRatio) const override {
            PYBIND11_OVERLOAD_PURE(Ray, Scene, primaryRay, rp, aspectRatio);
        }
        virtual std::optional<Vec2> rasterPosition(Vec3 wo, Float aspectRatio) const override {
            PYBIND11_OVERLOAD_PURE(std::optional<Vec2>, Scene, rasterPosition, wo, aspectRatio);
        }
        virtual std::optional<RaySample> sampleRay(Rng& rng, const SceneInteraction& sp, Vec3 wi) const override {
            PYBIND11_OVERLOAD_PURE(std::optional<RaySample>, Scene, sampleRay, rng, sp, wi);
        }
        virtual std::optional<Vec3> sampleDirectionGivenComp(Rng& rng, const SceneInteraction& sp, int comp, Vec3 wi) const {
            PYBIND11_OVERLOAD_PURE(std::optional<Vec3>, Scene, sampleDirectionGivenComp, rng, sp, comp, wi);
        }
        virtual std::optional<RaySample> sampleDirectLight(Rng& rng, const SceneInteraction& sp) const override {
            PYBIND11_OVERLOAD_PURE(std::optional<RaySample>, Scene, sampleDirectLight, rng, sp);
        }
        virtual Float pdf(const SceneInteraction& sp, int comp, Vec3 wi, Vec3 wo) const override {
            PYBIND11_OVERLOAD_PURE(Float, Scene, pdf, sp, comp, wi, wo);
        }
        virtual Float pdfComp(const SceneInteraction& sp, int comp, Vec3 wi) const override {
            PYBIND11_OVERLOAD_PURE(Float, Scene, pdfComp, sp, comp, wi);
        }
        virtual Float pdfDirectLight(const SceneInteraction& sp, const SceneInteraction& spL, int compL, Vec3 wo) const override {
            PYBIND11_OVERLOAD_PURE(Float, Scene, pdfDirectLight, sp, spL, compL, wo);
        }
        virtual std::optional<DistanceSample> sampleDistance(Rng& rng, const SceneInteraction& sp, Vec3 wo) const override {
            PYBIND11_OVERLOAD_PURE(std::optional<DistanceSample>, Scene, sampleDistance, rng, sp, wo);
        }
        virtual Vec3 evalTransmittance(Rng& rng, const SceneInteraction& sp1, const SceneInteraction& sp2) const override {
            PYBIND11_OVERLOAD_PURE(Vec3, Scene, evalTransmittance, rng, sp1, sp2);
        }
        virtual Vec3 evalContrb(const SceneInteraction& sp, int comp, Vec3 wi, Vec3 wo) const override {
            PYBIND11_OVERLOAD_PURE(Vec3, Scene, evalContrb, sp, comp, wi, wo);
        }
        virtual Vec3 evalContrbEndpoint(const SceneInteraction& sp, Vec3 wo) const override {
            PYBIND11_OVERLOAD_PURE(Vec3, Scene, evalContrbEndpoint, sp, wo);
        }
        virtual std::optional<Vec3> reflectance(const SceneInteraction& sp, int comp) const override {
            PYBIND11_OVERLOAD_PURE(std::optional<Vec3>, Scene, reflectance, sp, comp);
        }
    };
    pybind11::class_<Scene, Scene_Py, Component::Ptr<Scene>>(m, "Scene")
        .def(pybind11::init<>())
        .def("renderable", &Scene::renderable)
        .def("rootNode", &Scene::rootNode)
        .def("createNode", &Scene::createNode)
        .def("addChild", &Scene::addChild)
        .def("addChildFromModel", &Scene::addChildFromModel)
        .def("traversePrimitiveNodes", &Scene::traversePrimitiveNodes)
        .def("build", &Scene::build)
        .def("intersect", &Scene::intersect, "ray"_a = Ray{}, "tmin"_a = Eps, "tmax"_a = Inf)
        .def("isLight", &Scene::isLight)
        .def("isSpecular", &Scene::isSpecular)
        .def("primaryRay", &Scene::primaryRay)
        .def("sampleRay", &Scene::sampleRay)
        .def("evalContrbEndpoint", &Scene::evalContrbEndpoint)
        .def("reflectance", &Scene::reflectance)
        .PYLM_DEF_COMP_BIND(Scene);
}

// ------------------------------------------------------------------------------------------------

// Bind renderer.h
static void bind_renderer(pybind11::module& m) {
    class Renderer_Py final : public Renderer {
        PYLM_SERIALIZE_IMPL(Renderer)
            virtual void construct(const Json& prop) override {
            PYBIND11_OVERLOAD(void, Renderer, construct, prop);
        }
        virtual void render(const Scene* scene) const override {
            PYBIND11_OVERLOAD_PURE(void, Renderer, render, scene);
        }
    };
    pybind11::class_<Renderer, Renderer_Py, Component::Ptr<Renderer>>(m, "Renderer")
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
        virtual Vec3 evalByPixelCoords(int x, int y) const override {
            PYBIND11_OVERLOAD_PURE(Vec3, Texture, evalByPixelCoords, x, y);
        }
    };
    pybind11::class_<Texture, Texture_Py, Component::Ptr<Texture>>(m, "Texture")
        .def(pybind11::init<>())
        .def("size", &Texture::size)
        .def("eval", &Texture::eval)
        .def("evalByPixelCoords", &Texture::evalByPixelCoords)
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
        virtual bool isSpecular(const PointGeometry& geom, int comp) const override {
            PYBIND11_OVERLOAD_PURE(bool, Material, isSpecular, geom, comp);
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
        virtual Float pdfComp(const PointGeometry& geom, int comp, Vec3 wi) const override {
            PYBIND11_OVERLOAD_PURE(Float, Material, pdfComp, geom, comp, wi);
        }
        virtual Vec3 eval(const PointGeometry& geom, int comp, Vec3 wi, Vec3 wo) const override {
            PYBIND11_OVERLOAD_PURE(Vec3, Material, eval, geom, comp, wi, wo);
        }
    };
    pybind11::class_<Material, Material_Py, Component::Ptr<Material>>(m, "Material")
        .def(pybind11::init<>())
        .def("isSpecular", &Material::isSpecular)
        .def("sample", &Material::sample)
        .def("reflectance", &Material::reflectance)
        .def("pdf", &Material::pdf)
        .def("eval", &Material::eval)
        .PYLM_DEF_COMP_BIND(Material);
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
    bind_user(m);
    bind_logger(m);
    bind_parallel(m);
    bind_objloader(m);
    bind_progress(m);
    bind_debugio(m);
    bind_debug(m);
    bind_distributed(m);
    bind_film(m);
    bind_surface(m);
    bind_scene(m);
    bind_renderer(m);
    bind_texture(m);
    bind_material(m);
}

LM_NAMESPACE_END(LM_NAMESPACE)
