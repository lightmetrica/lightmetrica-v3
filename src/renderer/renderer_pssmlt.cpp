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
#include <lm/parallel.h>
#include <lm/bidir.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// State definition
class PSSMLTState {
private:
    // Variable type used to represent the state of subpaths
    using SubpathSampleU = std::vector<path::RaySampleU>;

private:
    int min_verts_;         // Minimum number of path vertices
    int max_verts_;         // Maximum number of path vertices
    SubpathSampleU usL_;    // State for light subpath
    SubpathSampleU usE_;    // State for eye subpath

public:
    PSSMLTState() = default;

    PSSMLTState(Rng& rng, int min_verts, int max_verts)
        : min_verts_(min_verts)
        , max_verts_(max_verts)
    {
        usL_.assign(max_verts_, {});
        usE_.assign(max_verts_, {});
        for (auto& u : usE_) u = rng.next<path::RaySampleU>();
        for (auto& u : usL_) u = rng.next<path::RaySampleU>();
    }

private:
    // Map a subpath from the state
    Path map_subpath(const SubpathSampleU& us, const Scene* scene, TransDir trans_dir, const SceneInteraction& sp_term) const {
        // Perform random walk
        Vec3 wi{};
        SceneInteraction sp = sp_term;
        Path path;
        for (int i = 0;; i++) {
            // Sample a ray
            const auto s = path::sample_ray(us[i], scene, sp, wi, trans_dir);
            if (!s || math::is_zero(s->weight)) {
                break;
            }

            // Add vertex
            path.vs.push_back({ s->sp, s->specular });
            if (path.num_verts() >= max_verts_) {
                break;
            }

            // Intersection to next surface
            const auto hit = scene->intersect(s->ray());
            if (!hit) {
                break;
            }

            // Update
            wi = -s->wo;
            sp = *hit;
        }
        return path;
    }

public:
    // Compute a path mapped from the state
    struct CachedPath {
        int s, t;
        Path path;
        Vec3 C_unweighted;  // Cached unweighed contribution
        Float w;            // Cached MIS weight
    };
    struct CachedPaths {
        std::vector<CachedPath> ps;
        Float scalar_contrb() const {
            Vec3 C(0_f);
            for (const auto& p : ps) C += p.C_unweighted * p.w;
            return path::scalar_contrb(C);
        }
    };
    CachedPaths map(const Scene* scene) const {
        // Map subpaths
        const auto subpathE = map_subpath(usE_, scene, TransDir::EL,
            SceneInteraction::make_camera_term());
        const auto subpathL = map_subpath(usL_, scene, TransDir::LE,
            SceneInteraction::make_light_term());

        // Number of vertices in each subpath
        const int nE = subpathE.num_verts();
        const int nL = subpathL.num_verts();

        // Generate full paths
        CachedPaths paths;
        for (int s = 0; s <= nL; s++) {
            for (int t = 0; t <= nE; t++) {
                const int k = s + t;
                if (k < min_verts_ || max_verts_ < k) {
                    continue;
                }

                // Connect subpaths
                const auto path = path::connect_subpaths(scene, subpathL, subpathE, s, t);
                if (!path) {
                    continue;
                }

                // Evaluate contribution and probability
                const auto f = path->eval_measurement_contrb_bidir(scene, s);
                if (math::is_zero(f)) {
                    continue;
                }
                const auto p = path->pdf_bidir(scene, s);
                if (p == 0_f) {
                    continue;
                }

                // Unweighted contribution
                const auto C_unweighted = f / p;
                
                // MIS weight
                const auto w = path->eval_mis_weight(scene, s);

                // Add generated path
                paths.ps.push_back({s, t, *path, C_unweighted, w});
            }
        }

        return paths;
    }

    // Mutate the state
    PSSMLTState mutate_large_step(Rng& rng) {
        return PSSMLTState(rng, min_verts_, max_verts_);
    }

    PSSMLTState mutate_small_step(Rng& rng, Float s1, Float s2) {
        // Perturb a single value
        const auto perturb = [&](Float u) -> Float {
            Float result;
            Float r = rng.u();
            if (r < .5_f) {
                r = r * 2_f;
                result = u + s2 * std::exp(-std::log(s2 / s1) * r);
                if (result > 1_f) {
                    result -= 1_f;
                }
            }
            else {
                r = (r - .5_f) * 2_f;
                result = u - s2 * std::exp(-std::log(s2 / s1) * r);
                if (result < 0_f) {
                    result += 1_f;
                }
            }
            return result;
        };

        // Number of states necessary for a vertex
        const int N = sizeof(path::RaySampleU) / sizeof(Float);

        // Perturb a subpath
        const auto perturb_subpath = [&](SubpathSampleU& prop_us, const SubpathSampleU& curr_us) {
            for (size_t i = 0; i < usE_.size(); i++) {
                for (int j = 0; j < N; j++) {
                    prop_us[i].data[j] = perturb(curr_us[i].data[j]);
                }
            }
        };

        PSSMLTState prop(*this);
        perturb_subpath(prop.usE_, usE_);
        perturb_subpath(prop.usL_, usL_);

        return prop;
    }
};

// ------------------------------------------------------------------------------------------------

// Simplify the code to BDPT for debugging.
#define PSSMLT_SIMPLIFY_TO_BDPT 0

// Primary sample space MLT (PSSMLT)
class Renderer_PSSMLT final : public Renderer {
private:
    Scene* scene_;                                  // Reference to scene asset
    Film* film_;                                    // Reference to film asset for output
    int min_verts_;                                 // Minimum number of path vertices
    int max_verts_;                                 // Maximum number of path vertices
    std::optional<unsigned int> seed_;              // Random seed
    Component::Ptr<scheduler::Scheduler> sched_;    // Scheduler for parallel processing
    Float normalization_;                           // Normalization factor
    Float large_step_prob_;                         // Large step probability
    Float s1_;
    Float s2_;
    
public:
    virtual void construct(const Json& prop) override {
        scene_ = json::comp_ref<Scene>(prop, "scene");
        film_ = json::comp_ref<Film>(prop, "output");
        scene_->camera()->set_aspect_ratio(film_->aspect());
        min_verts_ = json::value<int>(prop, "min_verts");
        max_verts_ = json::value<int>(prop, "max_verts");
        seed_ = json::value_or_none<unsigned int>(prop, "seed");
        const auto sched_name = json::value<std::string>(prop, "scheduler");
        sched_ = comp::create<scheduler::Scheduler>(
            "scheduler::spi::" + sched_name, make_loc("scheduler"), prop);
        normalization_ = json::value<Float>(prop, "normalization");
        large_step_prob_ = json::value<Float>(prop, "large_step_prob");
        s1_ = json::value<Float>(prop, "s1", 1_f / 256_f);
        s2_ = json::value<Float>(prop, "s2", 1_f / 16_f);
    }

public:
    virtual Json render() const override {
        scene_->require_renderable();
        film_->clear();
        const auto size = film_->size();

        // Random number generator for initialization
        Rng init_rng(seed_ ? *seed_ : math::rng_seed());

        // ----------------------------------------------------------------------------------------

        // TLS
        struct Context {
            Rng rng;
            struct CachedState {
                PSSMLTState state;
                PSSMLTState::CachedPaths paths;
            };
            CachedState curr;
        };
        std::vector<Context> contexts(parallel::num_threads());
        for (int i = 0; i < (int)(contexts.size()); i++) {
            auto& ctx = contexts[i];
            ctx.rng = Rng(init_rng.u_int());

            // Generate initial state with BDPT
            while (true) {
                PSSMLTState state(init_rng, min_verts_, max_verts_);
                const auto paths = state.map(scene_);
                if (paths.ps.empty()) {
                    continue;
                }
                ctx.curr.state = std::move(state);
                ctx.curr.paths = std::move(paths);
                break;
            }
        }

        // ----------------------------------------------------------------------------------------
        
        // Execute parallel process
        const auto processed = sched_->run([&](long long, long long, int threadid) {
            auto& ctx = contexts[threadid];

            // Mutation in primary sample space
            #if PSSMLT_SIMPLIFY_TO_BDPT
            const auto prop = ctx.curr.state.mutate_large_step(ctx.rng);
            #else
            const auto prop = ctx.rng.u() < large_step_prob_
                ? ctx.curr.state.mutate_large_step(ctx.rng)
                : ctx.curr.state.mutate_small_step(ctx.rng, s1_, s2_);
            #endif
            
            // Map to paths
            const auto curr_path = ctx.curr.paths;
            const auto prop_path = prop.map(scene_);

            // Determine next state
            #if PSSMLT_SIMPLIFY_TO_BDPT
            // Always accept
            ctx.curr.state = std::move(prop);
            ctx.curr.paths = std::move(prop_path);
            #else
            // Scalar contribution
            const auto curr_contrb = curr_path.scalar_contrb();
            const auto prop_contrb = prop_path.scalar_contrb();
            
            // MH update
            const auto A = curr_contrb == 0_f ? 0_f : std::min(1_f, prop_contrb / curr_contrb);
            if (ctx.rng.u() < A) {
                ctx.curr.state = std::move(prop);
                ctx.curr.paths = std::move(prop_path);
            }
            #endif

            // Accumulate contribution
            const auto I  = ctx.curr.paths.scalar_contrb();
            for (const auto& p : ctx.curr.paths.ps) {
                const auto rp = p.path.raster_position(scene_);
                const auto C = p.C_unweighted * p.w;
                #if PSSMLT_SIMPLIFY_TO_BDPT
                film_->splat(rp, C);
                #else
                film_->splat(rp, C * (normalization_ / I));
                #endif
            }
        });

        // ----------------------------------------------------------------------------------------

        // Rescale film
        film_->rescale(Float(size.w*size.h) / processed);

        return { {"processed", processed} };
    }
};

LM_COMP_REG_IMPL(Renderer_PSSMLT, "renderer::pssmlt");

LM_NAMESPACE_END(LM_NAMESPACE)
