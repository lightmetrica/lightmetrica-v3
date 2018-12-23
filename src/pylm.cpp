/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
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

    // math.h

    // Constants
    m.attr("Inf") = Inf;
    m.attr("Eps") = Eps;
    m.attr("Pi") = Pi;

    // Ray
    pybind11::class_<Ray>(m, "Ray")
        .def_readwrite("o", &Ray::o)
        .def_readwrite("d", &Ray::d);

	// Helper functions
	m.def("identity", []() -> Mat4 {
		return Mat4(1_f);
	});
	m.def("rotate", [](Float angle, Vec3 axis) -> Mat4 {
		return glm::rotate(angle, axis);
	});
	m.def("radians", &glm::radians<Float>);
	m.def("make_vec3", [](Float v1, Float v2, Float v3) -> Vec3 {
		return Vec3(v1, v2, v3);
	});

    // ------------------------------------------------------------------------

    // component.h

    // Trampoline class for lm::Component
    class Component_Py final : public Component {
    public:
        virtual bool construct(const Json& prop) override {
            PYBIND11_OVERLOAD(bool, Component, construct, prop);
        }
    };
    pybind11::class_<Component, Component_Py, Component::Ptr<Component>>(m, "Component")
        .def(pybind11::init<>())
        .def("construct", &Component::construct)
        .def("parent", &Component::parent)
        .PYLM_DEF_COMP_BIND(Component);

    {
        // Namespaces are handled as submodules
        auto sm = m.def_submodule("comp");
        auto sm_detail = sm.def_submodule("detail");

        // Component API
        sm_detail.def("loadPlugin", &comp::detail::loadPlugin);
        sm_detail.def("loadPlugins", &comp::detail::loadPlugins);
        sm_detail.def("unloadPlugins", &comp::detail::unloadPlugins);
        sm_detail.def("foreachRegistered", &comp::detail::foreachRegistered);
        sm_detail.def("printRegistered", &comp::detail::printRegistered);
    }

    // ------------------------------------------------------------------------

    // user.h
    m.def("init", &init);
    m.def("shutdown", &shutdown);
    m.def("asset", &asset);
    m.def("getAsset", &getAsset, pybind11::return_value_policy::reference);
    m.def("primitive", &primitive);
    m.def("primitives", &primitives);
    m.def("build", &build);
    m.def("render", &render);
    m.def("save", &save);
    m.def("buffer", &buffer);

    // ------------------------------------------------------------------------

    // logger.h
    {
        auto sm = m.def_submodule("log");

        // LogLevel
        pybind11::enum_<log::LogLevel>(sm, "LogLevel")
            .value("Debug", log::LogLevel::Debug)
            .value("Info", log::LogLevel::Info)
            .value("Warn", log::LogLevel::Warn)
            .value("Err", log::LogLevel::Err)
            .value("Progress", log::LogLevel::Progress)
            .value("ProgressEnd", log::LogLevel::ProgressEnd);

        // Log API
        sm.def("init", &log::init);
        sm.def("shutdown", &log::shutdown);
        using LogFuncPtr = void(*)(log::LogLevel, const char*, int, const char*);
        sm.def("log", (LogFuncPtr)&log::log);
        sm.def("updateIndentation", &log::updateIndentation);

        // Context
        using LoggerContext = log::detail::LoggerContext;
        class LoggerContext_Py final : public LoggerContext {
            virtual bool construct(const Json& prop) override {
                PYBIND11_OVERLOAD(bool, LoggerContext, construct, prop);
            }
            virtual void log(log::LogLevel level, const char* filename, int line, const char* message) override {
                PYBIND11_OVERLOAD_PURE(void, LoggerContext, log, level, filename, line, message);
            }
            virtual void updateIndentation(int n) override {
                PYBIND11_OVERLOAD_PURE(void, LoggerContext, updateIndentation, n);
            }
        };
        pybind11::class_<LoggerContext, LoggerContext_Py, Component::Ptr<LoggerContext>>(sm, "LoggerContext")
            .def(pybind11::init<>())
            .def("log", &LoggerContext::log)
            .def("updateIndentation", &LoggerContext::updateIndentation)
            .PYLM_DEF_COMP_BIND(LoggerContext);
    }

    // ------------------------------------------------------------------------

    // parallel.h
    {
        auto sm = m.def_submodule("parallel");

        // Parallel API
        sm.def("init", &parallel::init);
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

        // Context
        using ParallelContext = parallel::detail::ParallelContext;
        class ParallelContext_Py final : public ParallelContext {
            virtual bool construct(const Json& prop) override {
                PYBIND11_OVERLOAD(bool, ParallelContext, construct, prop);
            }
            virtual int numThreads() const override {
                PYBIND11_OVERLOAD_PURE(int, ParallelContext, numThreads);
            }
            virtual bool mainThread() const override {
                PYBIND11_OVERLOAD_PURE(bool, ParallelContext, mainThread);
            }
            virtual void foreach(long long numSamples, const parallel::ParallelProcessFunc& processFunc) const override {
                PYBIND11_OVERLOAD_PURE(void, ParallelContext, foreach, numSamples, processFunc);
            }
        };
        pybind11::class_<ParallelContext, ParallelContext_Py, Component::Ptr<ParallelContext>>(sm, "ParallelContext")
            .def(pybind11::init<>())
            .def("numThreads", &ParallelContext::numThreads)
            .def("mainThread", &ParallelContext::mainThread)
            .def("foreach", &ParallelContext::foreach)
            .PYLM_DEF_COMP_BIND(ParallelContext);
    }

    // ------------------------------------------------------------------------

    // progress.h
    {
        auto sm = m.def_submodule("progress");

        // Progress report API
        sm.def("init", &progress::init);
        sm.def("shutdown", &progress::shutdown);
        sm.def("start", &progress::start);
        sm.def("end", &progress::end);
        sm.def("update", &progress::update);

        // Context
        using ProgressContext = progress::detail::ProgressContext;
        class ProgressContext_Py final : public ProgressContext {
            virtual bool construct(const Json& prop) override {
                PYBIND11_OVERLOAD(bool, ProgressContext, construct, prop);
            }
            virtual void start(long long total) override {
                pybind11::gil_scoped_acquire acquire;
                PYBIND11_OVERLOAD_PURE(void, ProgressContext, start, total);
            }
            virtual void end() override {
                pybind11::gil_scoped_acquire acquire;
                PYBIND11_OVERLOAD_PURE(void, ProgressContext, end);
            }
            virtual void update(long long processed) override {
                pybind11::gil_scoped_acquire acquire;
                PYBIND11_OVERLOAD_PURE(void, ProgressContext, update, processed);
            }
        };
        pybind11::class_<ProgressContext, ProgressContext_Py, Component::Ptr<ProgressContext>>(sm, "ProgressContext")
            .def(pybind11::init<>())
            .def("start", &ProgressContext::start)
            .def("end", &ProgressContext::end)
            .def("update", &ProgressContext::update)
            .PYLM_DEF_COMP_BIND(ProgressContext);
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
            PYBIND11_OVERLOAD(bool, Film, construct, prop);
        }
        virtual FilmSize size() const override {
            PYBIND11_OVERLOAD_PURE(FilmSize, Film, size);
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
    };
    pybind11::class_<Film, Film_Py, Component::Ptr<Film>>(m, "Film")
        .def(pybind11::init<>())
        .def("size", &Film::size)
        .def("setPixel", &Film::setPixel)
        .def("save", &Film::save)
        .def("aspectRatio", &Film::aspectRatio)
        .def("buffer", &Film::buffer)
        .PYLM_DEF_COMP_BIND(Film);

    // ------------------------------------------------------------------------

    // scene.h

    // Surface point
    pybind11::class_<SurfacePoint>(m, "SurfacePoint")
        .def(pybind11::init<>())
        .def_readwrite("primitive", &SurfacePoint::primitive)
        .def_readwrite("comp", &SurfacePoint::comp)
        .def_readwrite("degenerated", &SurfacePoint::degenerated)
        .def_readwrite("p", &SurfacePoint::p)
        .def_readwrite("n", &SurfacePoint::n)
        .def_readwrite("t", &SurfacePoint::t)
        .def_readwrite("u", &SurfacePoint::u)
        .def_readwrite("v", &SurfacePoint::v)
        .def_readwrite("endpoint", &SurfacePoint::endpoint)
        .def("opposite", &SurfacePoint::opposite)
        .def("orthonormalBasis", &SurfacePoint::orthonormalBasis);
    m.def("geometryTerm", &geometryTerm);
    
    // RaySample
    pybind11::class_<RaySample>(m, "RaySample")
        .def_readwrite("sp", &RaySample::sp)
        .def_readwrite("wo", &RaySample::wo)
        .def_readwrite("weight", &RaySample::weight);

	// LightSample
	pybind11::class_<LightSample>(m, "LightSample")
		.def_readwrite("wo", &LightSample::wo)
		.def_readwrite("d", &LightSample::d)
		.def_readwrite("weight", &LightSample::weight);

    // Scene
    class Scene_Py final : public Scene {
        virtual bool construct(const Json& prop) override {
            PYBIND11_OVERLOAD(bool, Scene, construct, prop);
        }
        virtual bool loadPrimitive(const Component& assetGroup, Mat4 transform, const Json& prop) override {
            PYBIND11_OVERLOAD_PURE(bool, Scene, loadPrimitive, assetGroup, transform, prop);
        }
        virtual bool loadPrimitives(const Component& assetGroup, Mat4 transform, const std::string& modelName) override {
            PYBIND11_OVERLOAD_PURE(bool, Scene, loadPrimitives, assetGroup, transform, modelName);
        }
        virtual void foreachTriangle(const ProcessTriangleFunc& processTriangle) const override {
            PYBIND11_OVERLOAD_PURE(void, Scene, foreachTriangle, processTriangle);
        }
        virtual void build(const std::string& name) override {
            PYBIND11_OVERLOAD_PURE(void, Scene, build, name);
        }
        virtual std::optional<SurfacePoint> intersect(Ray ray, Float tmin, Float tmax) const override {
            PYBIND11_OVERLOAD_PURE(std::optional<SurfacePoint>, Scene, intersect, ray, tmin, tmax);
        }
        virtual bool isLight(const SurfacePoint& sp) const override {
            PYBIND11_OVERLOAD_PURE(bool, Scene, isLight, sp);
        }
        virtual bool isSpecular(const SurfacePoint& sp) const override {
            PYBIND11_OVERLOAD_PURE(bool, Scene, isSpecular, sp);
        }
        virtual Ray primaryRay(Vec2 rp) const override {
            PYBIND11_OVERLOAD_PURE(Ray, Scene, primaryRay, rp);
        }
        virtual std::optional<RaySample> sampleRay(Rng& rng, const SurfacePoint& sp, Vec3 wi) const override {
            PYBIND11_OVERLOAD_PURE(std::optional<RaySample>, Scene, sampleRay, rng, sp, wi);
        }
        virtual std::optional<RaySample> samplePrimaryRay(Rng& rng, Vec4 window) const override {
            PYBIND11_OVERLOAD_PURE(std::optional<RaySample>, Scene, samplePrimaryRay, rng, window);
        }
		virtual std::optional<LightSample> sampleLight(Rng& rng, const SurfacePoint& sp) const override {
			PYBIND11_OVERLOAD_PURE(std::optional<LightSample>, Scene, sampleLight, rng, sp);
		}
		virtual Vec3 evalBsdf(const SurfacePoint& sp, Vec3 wi, Vec3 wo) const override {
			PYBIND11_OVERLOAD_PURE(Vec3, Scene, evalBsdf, sp, wi, wo);
		}
        virtual Vec3 evalContrbEndpoint(const SurfacePoint& sp, Vec3 wo) const override {
            PYBIND11_OVERLOAD_PURE(Vec3, Scene, evalContrbEndpoint, sp, wo);
        }
        virtual std::optional<Vec3> reflectance(const SurfacePoint& sp) const override {
            PYBIND11_OVERLOAD_PURE(std::optional<Vec3>, Scene, reflectance, sp);
        }
    };
    pybind11::class_<Scene, Scene_Py, Component::Ptr<Scene>>(m, "Scene")
        .def(pybind11::init<>())
        .def("loadPrimitive", &Scene::loadPrimitive)
        .def("loadPrimitives", &Scene::loadPrimitives)
        .def("foreachTriangle", &Scene::foreachTriangle)
        .def("build", &Scene::build)
        .def("intersect", &Scene::intersect)
        .def("isLight", &Scene::isLight)
        .def("isSpecular", &Scene::isSpecular)
        .def("primaryRay", &Scene::primaryRay)
        .def("sampleRay", &Scene::sampleRay)
        .def("samplePrimaryRay", &Scene::samplePrimaryRay)
        .def("evalContrbEndpoint", &Scene::evalContrbEndpoint)
        .def("reflectance", &Scene::reflectance)
        .PYLM_DEF_COMP_BIND(Scene);

    // ------------------------------------------------------------------------

    // renderer.h
    class Renderer_Py final : public Renderer {
        virtual bool construct(const Json& prop) override {
            PYBIND11_OVERLOAD(bool, Renderer, construct, prop);
        }
        virtual void render(const Scene* scene) const override {
            PYBIND11_OVERLOAD_PURE(void, Renderer, render, scene);
        }
    };
    pybind11::class_<Renderer, Renderer_Py, Component::Ptr<Renderer>>(m, "Renderer")
        .def(pybind11::init<>())
        .def("render", &Renderer::render, pybind11::call_guard<pybind11::gil_scoped_release>())
        .PYLM_DEF_COMP_BIND(Renderer);
}

// ----------------------------------------------------------------------------

PYBIND11_MODULE(pylm, m) {
    m.doc() = R"x(
        pylm: Python binding of Lightmetrica.
    )x";
    bind(m);
}

LM_NAMESPACE_END(LM_NAMESPACE)
