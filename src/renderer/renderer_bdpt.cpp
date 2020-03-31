/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/renderer.h>
#include <lm/scene.h>
#include <lm/film.h>
#include <lm/scheduler.h>
#include <lm/bidir.h>
#include <lm/debug.h>

// Poll mutated paths
#define BDPT_POLL_PATHS 1

#if BDPT_POLL_PATHS
LM_NAMESPACE_BEGIN(nlohmann)
// Specialize adl_serializer for one-way automatic conversion from path to Json type
template <>
struct adl_serializer<lm::Path> {
    static void to_json(lm::Json& j, const lm::Path& path) {
        for (const auto& v : path.vs) {
            j.push_back(v.sp.geom.p);
        }
    }
};
LM_NAMESPACE_END(nlohmann)
#endif

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// Base class for the path-space based renderers
class Renderer_Path_Base : public Renderer {
public:
    Scene* scene_;                                  // Reference to scene asset
    Film* film_;                                    // Reference to film asset for output
    int min_verts_;                                 // Minimum number of path vertices
    int max_verts_;                                 // Maximum number of path vertices
    std::optional<unsigned int> seed_;              // Random seed
    Component::Ptr<scheduler::Scheduler> sched_;    // Scheduler for parallel processing

public:
    virtual void construct(const Json& prop) override {
        scene_ = json::comp_ref<Scene>(prop, "scene");
        film_ = json::comp_ref<Film>(prop, "output");
        scene_->camera()->set_aspect_ratio(film_->aspect());
        min_verts_ = json::value<int>(prop, "min_verts", 2);
        max_verts_ = json::value<int>(prop, "max_verts");
        seed_ = json::value_or_none<unsigned int>(prop, "seed");
        const auto sched_name = json::value<std::string>(prop, "scheduler");
        sched_ = comp::create<scheduler::Scheduler>(
            "scheduler::spi::" + sched_name, make_loc("scheduler"), prop);
    }
};

// ------------------------------------------------------------------------------------------------

// Implements naive path tracing using path structure
class Renderer_PT_Naive_Path final : public Renderer_Path_Base {
public:
    virtual Json render() const override {
        scene_->require_renderable();
        film_->clear();
        const auto size = film_->size();

        // Execute parallel process
        const auto processed = sched_->run([&](long long, long long sample_index, int threadid) {
            // Per-thread random number generator
            thread_local Rng rng(seed_ ? *seed_ + threadid : math::rng_seed());

            // Sample eye subpath
            const auto subpathE = path::sample_subpath(rng, scene_, max_verts_, TransDir::EL);
            const int nE = (int)(subpathE.vs.size());

            // Create full paths of all possible lengths
            for (int t = 2; t <= nE; t++) {
                const int nv = t;
                if (nv < min_verts_ || max_verts_ < nv) {
                    continue;
                }

                // Connect the subpaths. Note that light subpath is empty.
                const auto path = path::connect_subpaths(scene_, {}, subpathE, 0, t);
                if (!path) {
                    continue;
                }

                #if BDPT_POLL_PATHS
                if (threadid == 0) {
                    debug::poll({
                        {"id", "path"},
                        {"sample_index", sample_index},
                        {"path", *path}
                    });
                }
                #endif

                // Evaluate contribution
                const auto C = path->eval_unweighted_contrb_bidir(scene_, 0);
                if (math::is_zero(C)) {
                    continue;
                }

                // Accumulate contribution
                const auto rp = path->raster_position(scene_);
                film_->splat(rp, C);
            }
        });

        // Rescale film
        film_->rescale(Float(size.w * size.h) / processed);

        return { {"processed", processed} };
    }
};

LM_COMP_REG_IMPL(Renderer_PT_Naive_Path, "renderer::pt_naive_path");

// ------------------------------------------------------------------------------------------------

// Path tracing with NEE using path structure
class Renderer_PT_NEE_Path final : public Renderer_Path_Base {
public:
    virtual Json render() const override {
        scene_->require_renderable();
        film_->clear();
        const auto size = film_->size();

        // Execute parallel process
        const auto processed = sched_->run([&](long long, long long, int threadid) {
            // Per-thread random number generator
            thread_local Rng rng(seed_ ? *seed_ + threadid : math::rng_seed());

            // Sample subpaths
            const auto subpathE = path::sample_subpath(rng, scene_, max_verts_, TransDir::EL);
            const auto subpathL = path::sample_subpath(rng, scene_, 1, TransDir::LE);
            const int nE = (int)(subpathE.vs.size());
            const int nL = (int)(subpathL.vs.size());
            assert(nL > 0);

            // Create full paths
            for (int n = 2; n <= nL+nE; n++) {
                if (n < min_verts_ || max_verts_ < n) {
                    continue;
                }

                // Select strategy
                // If the path is not samplable by (1,k-1) strategy
                // use (0,k) strategy instead.
                int s = 1;
                int t = n-1;
                if (s >= nL || t >= nE) {
                    continue;
                }
                {
                    const auto& v = subpathE.vs[t-1];
                    if (path::is_specular_component(scene_, v.sp, v.comp)) {
                        s = 0;
                        t = n;
                    }
                }

                // Selected strategy is not sampled due to the early rejection
                if (s > nL || t > nE) {
                    continue;
                }

                // Connect subpaths
                const auto path = path::connect_subpaths(scene_, subpathL, subpathE, s, t);
                if (!path) {
                    continue;
                }

                // Evaluate contribution
                const auto C = path->eval_unweighted_contrb_bidir(scene_, s);
                if (math::is_zero(C)) {
                    continue;
                }

                // Accumulate contribution
                const auto rp = path->raster_position(scene_);
                film_->splat(rp, C);
            }
        });

        // Rescale film
        film_->rescale(Float(size.w * size.h) / processed);

        return { {"processed", processed} };
    }
};

LM_COMP_REG_IMPL(Renderer_PT_NEE_Path, "renderer::pt_nee_path");

// ------------------------------------------------------------------------------------------------

// Light trasing with NEE using path structure
class Renderer_LT_NEE_Path final : public Renderer_Path_Base {
public:
    virtual Json render() const override {
        scene_->require_renderable();
        film_->clear();
        const auto size = film_->size();

        // Execute parallel process
        const auto processed = sched_->run([&](long long, long long sample_index, int threadid) {
            // Per-thread random number generator
            thread_local Rng rng(seed_ ? *seed_ + threadid : math::rng_seed());

            // Sample subpaths
            const auto subpathE = path::sample_subpath(rng, scene_, 1, TransDir::EL);
            const auto subpathL = path::sample_subpath(rng, scene_, max_verts_, TransDir::LE);
            const int nE = (int)(subpathE.vs.size());
            assert(nE > 0);
            const int nL = (int)(subpathL.vs.size());

            // Create full paths
            for (int n = 2; n <= nL + nE; n++) {
                if (n < min_verts_ || max_verts_ < n) {
                    continue;
                }

                // Strategy index
                const int s = n - 1;
                const int t = 1;
                if (s >= nL || t >= nE) {
                    continue;
                }

                // Connect subpaths
                const auto path = path::connect_subpaths(scene_, subpathL, subpathE, s, t);
                if (!path) {
                    continue;
                }

                #if BDPT_POLL_PATHS
                if (threadid == 0) {
                    debug::poll({
                        {"id", "path"},
                        {"sample_index", sample_index},
                        {"path", *path}
                    });
                }
                #endif

                #if 1
                // Evaluate contribution
                const auto C = path->eval_unweighted_contrb_bidir(scene_, s);
                if (math::is_zero(C)) {
                    continue;
                }
                #else
                // Evaluate contribution and probability
                const auto f = path->eval_measurement_contrb_bidir(scene_, s);
                if (math::is_zero(f)) {
                    continue;
                }
                const auto p = path->pdf_bidir(scene_, s);
                if (p == 0_f) {
                    continue;
                }
                const auto C = f / p;
                #endif

                // Accumulate contribution
                const auto rp = path->raster_position(scene_);
                film_->splat(rp, C);
            }
        });

        // Rescale film
        film_->rescale(Float(size.w * size.h) / processed);

        return { {"processed", processed} };
    }
};

LM_COMP_REG_IMPL(Renderer_LT_NEE_Path, "renderer::lt_nee_path");

// ------------------------------------------------------------------------------------------------

// Enable the output of per-strategy film
#define BDPT_PER_STRATEGY_FILM 0
#define BDPT_SEPARATE_EVAL_UNWEIGHT_CONTRB 0

// Bidirectional path tracing
class Renderer_BDPT final : public Renderer_Path_Base {
public:
    #if BDPT_PER_STRATEGY_FILM
    // Index: (k, s)
    // where k = 2,..,max_verts,
    //       s = 0,...,s
    mutable std::vector<std::vector<Ptr<Film>>> strategy_films_;
    std::unordered_map<std::string, Film*> strategy_film_name_map_;

    virtual Component* underlying(const std::string& name) const override {
        return strategy_film_name_map_.at(name);
    }

    virtual void construct(const Json& prop) override {
        Renderer_Path_Base::construct(prop);
        const auto size = film_->size();
        for (int k = 2; k <= max_verts_; k++) {
            strategy_films_.emplace_back();
            for (int s = 0; s <= k; s++) {
                const auto name = fmt::format("film_{}_{}", k, s);
                auto film = comp::create<Film>("film::bitmap", make_loc(name), {
                    {"w", size.w},
                    {"h", size.h}
                });
                film->clear();
                strategy_film_name_map_[name] = film.get();
                strategy_films_.back().push_back(std::move(film));
            }
        }
    }
    #endif

public:
    virtual Json render() const override {
        scene_->require_renderable();
        film_->clear();
        const auto size = film_->size();

        // Execute parallel process
        const auto processed = sched_->run([&](long long, long long sample_index, int threadid) {
            LM_KEEP_UNUSED(sample_index);

            // Per-thread random number generator
            thread_local Rng rng(seed_ ? *seed_ + threadid : math::rng_seed());

            // Sample subpaths
            const auto subpathE = path::sample_subpath(rng, scene_, max_verts_, TransDir::EL);
            const auto subpathL = path::sample_subpath(rng, scene_, max_verts_, TransDir::LE);
            const int nE = subpathE.num_verts();
            const int nL = subpathL.num_verts();

            // Generate full paths
            for (int s = 0; s <= nL; s++) {
                for (int t = 0; t <= nE; t++) {
                    const int k = s + t;
                    if (k < min_verts_ || max_verts_ < k) {
                        continue;
                    }

                    // Connect subpaths
                    const auto path = path::connect_subpaths(scene_, subpathL, subpathE, s, t);
                    if (!path) {
                        continue;
                    }

                    #if BDPT_SEPARATE_EVAL_UNWEIGHT_CONTRB
                    // Evaluate contribution and probability
                    const auto f = path->eval_measurement_contrb_bidir(scene_, s);
                    if (math::is_zero(f)) {
                        continue;
                    }
                    const auto p = path->pdf_bidir(scene_, s);
                    if (p == 0_f) {
                        continue;
                    }
                    // Unweighted contribution
                    const auto C_unweighted = f / p;
                    #else
                    // Unweighted contribution
                    const auto C_unweighted = path->eval_unweighted_contrb_bidir(scene_, s);
                    if (math::is_zero(C_unweighted)) {
                        continue;
                    }
                    #endif

                    // MIS weight
                    const auto w = path->eval_mis_weight(scene_, s);
                    
                    // Accumulate contribution
                    const auto rp = path->raster_position(scene_);
                    const auto C = w * C_unweighted;
                    film_->splat(rp, C);
                    #if BDPT_PER_STRATEGY_FILM
                    auto& strategy_film = strategy_films_[k-2][s];
                    strategy_film->splat(rp, C_unweighted);
                    #endif
                }
            }
        });

        // Rescale film
        const auto scale = Float(size.w * size.h) / processed;
        film_->rescale(scale);
        #if BDPT_PER_STRATEGY_FILM
        for (int k = 2; k <= max_verts_; k++) {
            for (int s = 0; s <= k; s++) {
                strategy_films_[k-2][s]->rescale(scale);
            }
        }
        #endif

        return { {"processed", processed} };
    }
};

LM_COMP_REG_IMPL(Renderer_BDPT, "renderer::bdpt");

LM_NAMESPACE_END(LM_NAMESPACE)
