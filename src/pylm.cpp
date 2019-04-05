/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>
#include <lm/pylm.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \brief Binds Lightmetrica to a Python module.
*/
static void bind(pybind11::module& m) {
    using namespace pybind11::literals;

    // ------------------------------------------------------------------------

    // Build parameters
    // Debug or Release mode
    m.attr("Debug") = LM_DEBUG_MODE ? true : false;

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

    // ------------------------------------------------------------------------
    
    // version.h
    {
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

    // ------------------------------------------------------------------------

    // math.h

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

    {
        auto sm = m.def_submodule("math");
        sm.def("orthonormalBasis", &math::orthonormalBasis, PYLM_SCOPED_RELEASE);
        sm.def("safeSqrt", &math::safeSqrt, PYLM_SCOPED_RELEASE);
        sm.def("sq", &math::sq, PYLM_SCOPED_RELEASE);
        sm.def("reflection", &math::reflection, PYLM_SCOPED_RELEASE);
        pybind11::class_<math::RefractionResult>(sm, "RefractionResult")
            .def_readwrite("wt", &math::RefractionResult::wt)
            .def_readwrite("total", &math::RefractionResult::total);
        sm.def("refraction", &math::refraction, PYLM_SCOPED_RELEASE);
        sm.def("sampleCosineWeighted", &math::sampleCosineWeighted, PYLM_SCOPED_RELEASE);
        sm.def("balanceHeuristic", &math::balanceHeuristic, PYLM_SCOPED_RELEASE);
    }

    // ------------------------------------------------------------------------

    // component.h

    class Component_Py final : public Component {
    public:
        virtual bool construct(const Json& prop) override {
            PYBIND11_OVERLOAD(bool, Component, construct, prop);
        }
    };
    pybind11::class_<Component, Component_Py, Component::Ptr<Component>>(m, "Component")
        .def(pybind11::init<>())
        .def("key", &Component::key, PYLM_SCOPED_RELEASE)
        .def("loc", &Component::loc, PYLM_SCOPED_RELEASE)
        .def("parentLoc", &Component::parentLoc, PYLM_SCOPED_RELEASE)
        .def("construct", &Component::construct, PYLM_SCOPED_RELEASE)
        .PYLM_DEF_COMP_BIND(Component);

    {
        auto sm = m.def_submodule("comp");
        sm.def("get", [](const std::string& locator) -> Component* {
            return comp::get<Component>(locator);
        }, pybind11::return_value_policy::reference);
        {
            auto sm_detail = sm.def_submodule("detail");
            sm_detail.def("loadPlugin", &comp::detail::loadPlugin, PYLM_SCOPED_RELEASE);
            sm_detail.def("loadPlugins", &comp::detail::loadPlugins, PYLM_SCOPED_RELEASE);
            sm_detail.def("unloadPlugins", &comp::detail::unloadPlugins, PYLM_SCOPED_RELEASE);
            sm_detail.def("foreachRegistered", &comp::detail::foreachRegistered, PYLM_SCOPED_RELEASE);
        }
    }

    // ------------------------------------------------------------------------

    // user.h
    m.def("init", &init, "type"_a = user::DefaultType, "prop"_a = Json{}, PYLM_SCOPED_RELEASE);
    m.def("shutdown", &shutdown, PYLM_SCOPED_RELEASE);
    m.def("info", &info, PYLM_SCOPED_RELEASE);
    m.def("reset", &reset, PYLM_SCOPED_RELEASE);
    m.def("asset", (std::string(*)(const std::string&, const std::string&, const Json&))&asset, PYLM_SCOPED_RELEASE);
    m.def("asset", (std::string(*)(const std::string&))&asset, PYLM_SCOPED_RELEASE);
    m.def("primitive", &primitive, PYLM_SCOPED_RELEASE);
    m.def("build", &build, PYLM_SCOPED_RELEASE);
    m.def("renderer", &renderer, PYLM_SCOPED_RELEASE);
    m.def("render", (void(*)(bool))&render, "verbose"_a = true, pybind11::call_guard<pybind11::gil_scoped_release>());
    m.def("render", (void(*)(const std::string&, const Json&))&render, pybind11::call_guard<pybind11::gil_scoped_release>());
    m.def("save", &save, PYLM_SCOPED_RELEASE);
    m.def("buffer", &buffer, PYLM_SCOPED_RELEASE);
    m.def("serialize", (void(*)(const std::string&))&serialize, PYLM_SCOPED_RELEASE);
    m.def("deserialize", (void(*)(const std::string&))&deserialize, PYLM_SCOPED_RELEASE);
    m.def("validate", &validate, PYLM_SCOPED_RELEASE);

    // ------------------------------------------------------------------------

    // logger.h
    {
        auto sm = m.def_submodule("log");

        pybind11::enum_<log::LogLevel>(sm, "LogLevel")
            .value("Debug", log::LogLevel::Debug)
            .value("Info", log::LogLevel::Info)
            .value("Warn", log::LogLevel::Warn)
            .value("Err", log::LogLevel::Err)
            .value("Progress", log::LogLevel::Progress)
            .value("ProgressEnd", log::LogLevel::ProgressEnd);

        sm.def("init", &log::init, "type"_a = log::DefaultType, "prop"_a = Json{}, PYLM_SCOPED_RELEASE);
        sm.def("shutdown", &log::shutdown, PYLM_SCOPED_RELEASE);
        using logFuncPtr = void(*)(log::LogLevel, int, const char*, int, const char*);
        sm.def("log", (logFuncPtr)&log::log, PYLM_SCOPED_RELEASE);
        sm.def("updateIndentation", &log::updateIndentation, PYLM_SCOPED_RELEASE);
        using setSeverityFuncPtr = void(*)(int);
        sm.def("setSeverity", (setSeverityFuncPtr)&log::setSeverity, PYLM_SCOPED_RELEASE);

        using LoggerContext = log::detail::LoggerContext;
        class LoggerContext_Py final : public LoggerContext {
            virtual bool construct(const Json& prop) override {
                pybind11::gil_scoped_acquire acquire;
                PYBIND11_OVERLOAD(bool, LoggerContext, construct, prop);
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
            .def("log", &LoggerContext::log, PYLM_SCOPED_RELEASE)
            .def("updateIndentation", &LoggerContext::updateIndentation, PYLM_SCOPED_RELEASE)
            .PYLM_DEF_COMP_BIND(LoggerContext);
    }

    // ------------------------------------------------------------------------

    // parallel.h
    {
        auto sm = m.def_submodule("parallel");
        sm.def("init", &parallel::init, "type"_a = parallel::DefaultType, "prop"_a = Json{}, PYLM_SCOPED_RELEASE);
        sm.def("shutdown", &parallel::shutdown, PYLM_SCOPED_RELEASE);
        sm.def("numThreads", &parallel::numThreads, PYLM_SCOPED_RELEASE);
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

    // ------------------------------------------------------------------------

    // objloader.h
    {
        auto sm = m.def_submodule("objloader");
        sm.def("init", &objloader::init, PYLM_SCOPED_RELEASE);
        sm.def("shutdown", &objloader::shutdown, PYLM_SCOPED_RELEASE);
    }

    // ------------------------------------------------------------------------

    // progress.h
    {
        auto sm = m.def_submodule("progress");

        sm.def("init", &progress::init, "type"_a = progress::DefaultType, "prop"_a = Json{}, PYLM_SCOPED_RELEASE);
        sm.def("shutdown", &progress::shutdown, PYLM_SCOPED_RELEASE);
        sm.def("start", &progress::start, PYLM_SCOPED_RELEASE);
        sm.def("end", &progress::end, PYLM_SCOPED_RELEASE);
        sm.def("update", &progress::update, PYLM_SCOPED_RELEASE);

        using ProgressContext = progress::detail::ProgressContext;
        class ProgressContext_Py final : public ProgressContext {
            virtual bool construct(const Json& prop) override {
                PYLM_ACQUIRE_GIL();
                PYBIND11_OVERLOAD(bool, ProgressContext, construct, prop);
            }
            virtual void start(long long total) override {
                PYLM_ACQUIRE_GIL();
                PYBIND11_OVERLOAD_PURE(void, ProgressContext, start, total);
            }
            virtual void end() override {
                PYLM_ACQUIRE_GIL();
                PYBIND11_OVERLOAD_PURE(void, ProgressContext, end);
            }
            virtual void update(long long processed) override {
                PYLM_ACQUIRE_GIL();
                PYBIND11_OVERLOAD_PURE(void, ProgressContext, update, processed);
            }
        };
        pybind11::class_<ProgressContext, ProgressContext_Py, Component::Ptr<ProgressContext>>(sm, "ProgressContext")
            .def(pybind11::init<>())
            .def("start", &ProgressContext::start, PYLM_SCOPED_RELEASE)
            .def("end", &ProgressContext::end, PYLM_SCOPED_RELEASE)
            .def("update", &ProgressContext::update, PYLM_SCOPED_RELEASE)
            .PYLM_DEF_COMP_BIND(ProgressContext);
    }

    // ------------------------------------------------------------------------

    // debugio.h
    {
        auto sm = m.def_submodule("debugio");
        sm.def("init", &debugio::init, PYLM_SCOPED_RELEASE);
        sm.def("shutdown", &debugio::shutdown, PYLM_SCOPED_RELEASE);
        sm.def("handleMessage", &debugio::handleMessage, PYLM_SCOPED_RELEASE);
        sm.def("syncUserContext", &debugio::syncUserContext, PYLM_SCOPED_RELEASE);
        sm.def("draw", &debugio::draw, PYLM_SCOPED_RELEASE);
    }

    // ------------------------------------------------------------------------

    // dist.h
    {
        auto sm = m.def_submodule("dist");
        sm.def("init", &dist::init, PYLM_SCOPED_RELEASE);
        sm.def("shutdown", &dist::shutdown, PYLM_SCOPED_RELEASE);
        sm.def("printWorkerInfo", &dist::printWorkerInfo, PYLM_SCOPED_RELEASE);
        sm.def("sync", &dist::sync, pybind11::call_guard<pybind11::gil_scoped_release>());
        sm.def("gatherFilm", &dist::gatherFilm, pybind11::call_guard<pybind11::gil_scoped_release>());
        {
            auto sm2 = sm.def_submodule("worker");
            sm2.def("init", &dist::worker::init, PYLM_SCOPED_RELEASE);
            sm2.def("shutdown", &dist::worker::shutdown, PYLM_SCOPED_RELEASE);
            sm2.def("run", &dist::worker::run, pybind11::call_guard<pybind11::gil_scoped_release>());
        }
    }

    // ------------------------------------------------------------------------

    // film.h

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
        virtual bool construct(const Json& prop) override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD(bool, Film, construct, prop);
        }
        virtual FilmSize size() const override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD_PURE(FilmSize, Film, size);
        }
        virtual void setPixel(int x, int y, Vec3 v) override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD_PURE(void, Film, setPixel, x, y, v);
        }
        virtual bool save(const std::string& outpath) const override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD_PURE(bool, Film, save, outpath);
        }
        virtual FilmBuffer buffer() override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD_PURE(FilmBuffer, Film, buffer);
        }
        virtual void accum(const Film* film) override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD_PURE(void, Film, accum, film);
        }
        virtual void clear() override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD_PURE(void, Film, clear);
        }
    };
    pybind11::class_<Film, Film_Py, Component::Ptr<Film>>(m, "Film")
        .def(pybind11::init<>())
        .def("size", &Film::size, PYLM_SCOPED_RELEASE)
        .def("setPixel", &Film::setPixel, PYLM_SCOPED_RELEASE)
        .def("save", &Film::save, PYLM_SCOPED_RELEASE)
        .def("aspectRatio", &Film::aspectRatio, PYLM_SCOPED_RELEASE)
        .def("buffer", &Film::buffer, PYLM_SCOPED_RELEASE)
        .PYLM_DEF_COMP_BIND(Film);

    // ------------------------------------------------------------------------

    // surface.h
    
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
        .def_static("makeDegenerated", &PointGeometry::makeDegenerated, PYLM_SCOPED_RELEASE)
        .def_static("makeInfinite", &PointGeometry::makeInfinite, PYLM_SCOPED_RELEASE)
        .def_static("makeOnSurface", (PointGeometry(*)(Vec3, Vec3, Vec2))&PointGeometry::makeOnSurface, PYLM_SCOPED_RELEASE)
        .def_static("makeOnSurface", (PointGeometry(*)(Vec3, Vec3))&PointGeometry::makeOnSurface, PYLM_SCOPED_RELEASE)
        .def("opposite", &PointGeometry::opposite, PYLM_SCOPED_RELEASE)
        .def("orthonormalBasis", &PointGeometry::orthonormalBasis, PYLM_SCOPED_RELEASE);

    pybind11::class_<SurfacePoint>(m, "SurfacePoint")
        .def(pybind11::init<>())
        .def_readwrite("primitive", &SurfacePoint::primitive)
        .def_readwrite("comp", &SurfacePoint::comp)
        .def_readwrite("geom", &SurfacePoint::geom)
        .def_readwrite("endpoint", &SurfacePoint::endpoint);

    {
        auto sm = m.def_submodule("surface");
        sm.def("geometryTerm", &surface::geometryTerm, PYLM_SCOPED_RELEASE);
    }

    // ------------------------------------------------------------------------

    // scene.h

    pybind11::class_<RaySample>(m, "RaySample")
        .def_readwrite("sp", &RaySample::sp)
        .def_readwrite("wo", &RaySample::wo)
        .def_readwrite("weight", &RaySample::weight)
        .def("ray", &RaySample::ray, PYLM_SCOPED_RELEASE);

    class Scene_Py final : public Scene {
        virtual bool construct(const Json& prop) override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD(bool, Scene, construct, prop);
        }
        virtual bool renderable() const override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD_PURE(bool, Scene, renderable);
        }
        virtual bool loadPrimitive(Mat4 transform, const Json& prop) override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD_PURE(bool, Scene, loadPrimitive, transform, prop);
        }
        virtual void foreachTriangle(const ProcessTriangleFunc& processTriangle) const override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD_PURE(void, Scene, foreachTriangle, processTriangle);
        }
        virtual void foreachPrimitive(const ProcessPrimitiveFunc& processPrimitive) const override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD_PURE(void, Scene, foreachPrimitive, processPrimitive);
        }
        virtual void build(const std::string& name, const Json& prop) override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD_PURE(void, Scene, build, name, prop);
        }
        virtual std::optional<SurfacePoint> intersect(Ray ray, Float tmin, Float tmax) const override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD_PURE(std::optional<SurfacePoint>, Scene, intersect, ray, tmin, tmax);
        }
        virtual bool isLight(const SurfacePoint& sp) const override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD_PURE(bool, Scene, isLight, sp);
        }
        virtual bool isSpecular(const SurfacePoint& sp) const override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD_PURE(bool, Scene, isSpecular, sp);
        }
        virtual Ray primaryRay(Vec2 rp, Float aspectRatio) const override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD_PURE(Ray, Scene, primaryRay, rp, aspectRatio);
        }
        virtual std::optional<RaySample> sampleRay(Rng& rng, const SurfacePoint& sp, Vec3 wi) const override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD_PURE(std::optional<RaySample>, Scene, sampleRay, rng, sp, wi);
        }
        virtual std::optional<RaySample> samplePrimaryRay(Rng& rng, Vec4 window, Float aspectRatio) const override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD_PURE(std::optional<RaySample>, Scene, samplePrimaryRay, rng, window, aspectRatio);
        }
        virtual std::optional<RaySample> sampleLight(Rng& rng, const SurfacePoint& sp) const override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD_PURE(std::optional<RaySample>, Scene, sampleLight, rng, sp);
        }
        virtual Float pdf(const SurfacePoint& sp, Vec3 wi, Vec3 wo) const override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD_PURE(Float, Scene, pdf, sp, wi, wo);
        }
        virtual Float pdfLight(const SurfacePoint& sp, const SurfacePoint& spL, Vec3 wo) const override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD_PURE(Float, Scene, pdfLight, sp, spL, wo);
        }
        virtual Vec3 evalBsdf(const SurfacePoint& sp, Vec3 wi, Vec3 wo) const override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD_PURE(Vec3, Scene, evalBsdf, sp, wi, wo);
        }
        virtual Vec3 evalContrbEndpoint(const SurfacePoint& sp, Vec3 wo) const override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD_PURE(Vec3, Scene, evalContrbEndpoint, sp, wo);
        }
        virtual std::optional<Vec3> reflectance(const SurfacePoint& sp) const override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD_PURE(std::optional<Vec3>, Scene, reflectance, sp);
        }
    };
    pybind11::class_<Scene, Scene_Py, Component::Ptr<Scene>>(m, "Scene")
        .def(pybind11::init<>())
        .def("loadPrimitive", &Scene::loadPrimitive, PYLM_SCOPED_RELEASE)
        .def("foreachTriangle", &Scene::foreachTriangle, PYLM_SCOPED_RELEASE)
        .def("foreachPrimitive", &Scene::foreachPrimitive, PYLM_SCOPED_RELEASE)
        .def("build", &Scene::build, PYLM_SCOPED_RELEASE)
        .def("intersect", &Scene::intersect, "ray"_a = Ray{}, "tmin"_a = Eps, "tmax"_a = Inf, PYLM_SCOPED_RELEASE)
        .def("isLight", &Scene::isLight, PYLM_SCOPED_RELEASE)
        .def("isSpecular", &Scene::isSpecular, PYLM_SCOPED_RELEASE)
        .def("primaryRay", &Scene::primaryRay, PYLM_SCOPED_RELEASE)
        .def("sampleRay", &Scene::sampleRay, PYLM_SCOPED_RELEASE)
        .def("samplePrimaryRay", &Scene::samplePrimaryRay, PYLM_SCOPED_RELEASE)
        .def("evalContrbEndpoint", &Scene::evalContrbEndpoint, PYLM_SCOPED_RELEASE)
        .def("reflectance", &Scene::reflectance, PYLM_SCOPED_RELEASE)
        .PYLM_DEF_COMP_BIND(Scene);

    // ------------------------------------------------------------------------

    // renderer.h
    class Renderer_Py final : public Renderer {
        virtual bool construct(const Json& prop) override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD(bool, Renderer, construct, prop);
        }
        virtual void render(const Scene* scene) const override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD_PURE(void, Renderer, render, scene);
        }
    };
    pybind11::class_<Renderer, Renderer_Py, Component::Ptr<Renderer>>(m, "Renderer")
        .def(pybind11::init<>())
        .def("render", &Renderer::render, PYLM_SCOPED_RELEASE)
        .PYLM_DEF_COMP_BIND(Renderer);

    // ------------------------------------------------------------------------

    // material.h

    pybind11::class_<MaterialDirectionSample>(m, "MaterialDirectionSample")
        .def(pybind11::init<>())
        .def_readwrite("wo", &MaterialDirectionSample::wo)
        .def_readwrite("comp", &MaterialDirectionSample::comp)
        .def_readwrite("weight", &MaterialDirectionSample::weight);

    class Material_Py final : public Material {
        virtual bool construct(const Json& prop) override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD(bool, Material, construct, prop);
        }
        virtual bool isSpecular(const PointGeometry& geom, int comp) const override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD(bool, Material, isSpecular, geom, comp);
        }
        virtual std::optional<MaterialDirectionSample> sample(Rng& rng, const PointGeometry& geom, Vec3 wi) const override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD(std::optional<MaterialDirectionSample>, Material, sample, rng, geom, wi);
        }
        virtual std::optional<Vec3> reflectance(const PointGeometry& geom, int comp) const override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD(std::optional<Vec3>, Material, reflectance, geom, comp);
        }
        virtual Float pdf(const PointGeometry& geom, int comp, Vec3 wi, Vec3 wo) const override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD(Float, Material, pdf, geom, comp, wi, wo);
        }
        virtual Vec3 eval(const PointGeometry& geom, int comp, Vec3 wi, Vec3 wo) const override {
            PYLM_ACQUIRE_GIL();
            PYBIND11_OVERLOAD(Vec3, Material, eval, geom, comp, wi, wo);
        }
    };
    pybind11::class_<Material, Material_Py, Component::Ptr<Material>>(m, "Material")
        .def(pybind11::init<>())
        .def("isSpecular", &Material::isSpecular, PYLM_SCOPED_RELEASE)
        .def("sample", &Material::sample, PYLM_SCOPED_RELEASE)
        .def("reflectance", &Material::reflectance, PYLM_SCOPED_RELEASE)
        .def("pdf", &Material::pdf, PYLM_SCOPED_RELEASE)
        .def("eval", &Material::eval, PYLM_SCOPED_RELEASE)
        .PYLM_DEF_COMP_BIND(Material);
}

// ----------------------------------------------------------------------------

PYBIND11_MODULE(pylm, m) {
    m.doc() = R"x(
        pylm: Python binding of Lightmetrica.
    )x";
    bind(m);
}

LM_NAMESPACE_END(LM_NAMESPACE)
