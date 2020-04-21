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
#include <lm/path.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

namespace {

// Path vertex with cache
struct Vert {
    SceneInteraction sp;    // Surface interaction
    int comp;               // Component index

                            // Cached variables
    Vec3 alpha;             // Path throughput up to this vertex
    Vec3 w_fwd;             // Outgoing direction
    Vec3 w_rev;             // Incoming direction
    Float pdf_fwd;          // PDF p(x_i | x_{i-1},x_{i-2})
    Float pdf_rev;          // PDF p(x_i | x_{i+1},x_{i+2})
};

// Light transport path
struct Path {
    std::vector<Vert> vs;   // Path vertices

    int num_verts() const {
        return (int)(vs.size());
    }

    int index(int i, TransDir trans_dir) const {
        return trans_dir == TransDir::LE ? i : num_verts() - 1 - i;
    }

    const Vert* vertex_at(int i, TransDir trans_dir) const {
        return (0 <= i && i < num_verts()) ? &vs[index(i, trans_dir)] : nullptr;
    }

    Vert* vertex_at(int i, TransDir trans_dir) {
        return (0 <= i && i < num_verts()) ? &vs[index(i, trans_dir)] : nullptr;
    }

    bool is_samplable_bidir(const Scene* scene, int s) const {
        const int n = num_verts();
        const int t = n - s;
        if (s == 0) {
            // If the vertex is not degenerated and non-specular, the endpoint is samplable
            const auto* vL = vertex_at(0, TransDir::LE);
            return !vL->sp.geom.degenerated &&
                   !path::is_specular_component(scene, vL->sp, vL->comp);
        }
        else if (t == 0) {
            // If the vertex is not degenerated and non-specular, the endpoint is samplable
            const auto* vE = vertex_at(0, TransDir::EL);
            return !vE->sp.geom.degenerated &&
                   !path::is_specular_component(scene, vE->sp, vE->comp);
        }
        else {
            const auto* vL = vertex_at(s-1, TransDir::LE);
            const auto* vE = vertex_at(t-1, TransDir::EL);
            if (s == 1 && !path::is_connectable_endpoint(scene, vL->sp)) {
                // Not samplebale if the endpoint is not connectable
                return false;
            }
            else if (t == 1 && !path::is_connectable_endpoint(scene, vE->sp)) {
                // Not samplebale if the endpoint is not connectable
                return false;
            }
            // Not samplable if either vL or vE is specular component
            if (path::is_specular_component(scene, vL->sp, vL->comp) ||
                path::is_specular_component(scene, vE->sp, vE->comp)) {
                return false;
            }
            return true;
        }
    }
};

// Direction from v_from to v_to
Vec3 direction(const Vert* v_from, const Vert* v_to) {
    if (v_from == nullptr || v_to == nullptr) {
        return {};
    }
    assert(!v_from->sp.geom.infinite || !v_to->sp.geom.infinite);
    if (v_from->sp.geom.infinite) {
        return v_from->sp.geom.wo;
    }
    else if (v_to->sp.geom.infinite) {
        return -v_to->sp.geom.wo;
    }
    else {
        return glm::normalize(v_to->sp.geom.p - v_from->sp.geom.p);
    }
}

// Sample subpath
void sample_subpath(Path& path, Rng& rng, const Scene* scene, int max_verts, TransDir trans_dir) {
    path.vs.clear();
    Vec3 throughput;
    Float pdf_fwd_next;

    // Perform random walk
    while (path.num_verts() < max_verts) {
        if (path.num_verts() == 0) {
            // Sample primary ray
            const auto s = path::sample_primary_ray(rng, scene, trans_dir);
            if (!s) {
                break;
            }

            // Add initial vertex
            Vert v;
            v.sp = s->sp;
            v.comp = 0;
            v.pdf_fwd = [&]() -> Float {
                if (path::is_connectable_endpoint(scene, s->sp)) {
                    const auto pA = path::pdf_position(scene, s->sp);
                    return pA;
                }
                else {
                    return 1_f;
                }
            }();
            v.alpha = Vec3(1_f/v.pdf_fwd);
            v.w_rev = {};
            v.w_fwd = s->wo;

            // Update information for the next vertex
            throughput = s->weight;
            pdf_fwd_next = [&]() -> Float {
                if (path::is_connectable_endpoint(scene, v.sp)) {
                    return path::pdf_direction(scene, v.sp, {}, v.w_fwd, v.comp, false);
                }
                else {
                    return path::pdf_primary_ray(scene, v.sp, v.w_fwd, false);
                }
            }();

            path.vs.push_back(v);
        }
        else {
            // Sample direction
            auto& v = path.vs[path.num_verts()-1];
            const auto s = path::sample_direction(rng, scene, v.sp, v.w_rev, v.comp, trans_dir);
            if (!s) {
                break;
            }

            // Update cached information
            v.w_fwd = s->wo;
            auto& v_prev = path.vs[path.num_verts()-2];
            v_prev.pdf_rev = v_prev.sp.geom.degenerated ? 1_f /*undefined*/ :
                surface::convert_pdf_to_area(
                    path::pdf_direction(scene, v.sp, v.w_fwd, v.w_rev, v.comp, false),
                    v.sp.geom, v_prev.sp.geom);

            // Update information for the next vertex
            throughput *= s->weight;
            pdf_fwd_next = path::pdf_direction(scene, v.sp, v.w_rev, v.w_fwd, v.comp, false);
        }

        // Current vertex
        auto& v = path.vs[path.num_verts()-1];

        // Intersection to next surface
        const auto hit = scene->intersect({ v.sp.geom.p, v.w_fwd });
        if (!hit) {
            break;
        }

        // Sample component
        const auto s_comp = path::sample_component(rng, scene, *hit);
        throughput *= s_comp.weight;

        // Create a vertex
        Vert v_next;
        v_next.sp = *hit;
        v_next.comp = s_comp.comp;
        v_next.alpha = throughput;
        v_next.w_rev = -v.w_fwd;
        v_next.pdf_fwd = surface::convert_pdf_to_area(pdf_fwd_next, v.sp.geom, hit->geom);
        path.vs.push_back(v_next);

        // Termination
        if (hit->geom.infinite) {
            break;
        }
    }
}

// Connect subpaths and evaluate contribution
// Returns nullopt if the connected paths gives zero-contribution.
struct Splat {
    Vec3 C;
    Vec2 rp;
};
std::optional<Splat> connect_and_eval_contrb(const Scene* scene, const Path& subpathE, const Path& subpathL, int s, int t) {
    assert(s >= 0 && t >= 0);
    assert(!(s == 0 && t == 0));
    assert(s + t >= 2);
    
    // Check if the path is samplable by the strategy (s,t)
    if (s == 0) {
        const auto& vL = subpathE.vs[t-1];
        if (vL.sp.geom.degenerated) {
            return {};
        }
    }
    else if (t == 0) {
        const auto& vE = subpathL.vs[s-1];
        if (vE.sp.geom.degenerated) {
            return {};
        }
    }
    else {
        const auto& vL = subpathL.vs[s-1];
        const auto& vE = subpathE.vs[t-1];
        if (s == 1 && !path::is_connectable_endpoint(scene, vL.sp)) {
            return {};
        }
        else if (t == 1 && !path::is_connectable_endpoint(scene, vE.sp)) {
            return {};
        }
        if (vL.sp.geom.infinite || vE.sp.geom.infinite) {
            return {};
        }
        if (path::is_specular_component(scene, vL.sp, vL.comp) ||
            path::is_specular_component(scene, vE.sp, vE.comp)) {
            return {};
        }
    }

    // Compute contribution
    Vec3 C;
    if (s == 0) {
        const auto& vL = subpathE.vs[t-1];
        const auto spL = vL.sp.as_type(SceneInteraction::LightEndpoint);
        const auto Le = path::eval_contrb_direction(scene, spL, {}, vL.w_rev, {}, TransDir::LE, true);
        C = Le * vL.alpha;
    }
    else if (t == 0) {
        const auto& vE = subpathL.vs[s-1];
        const auto spE = vE.sp.as_type(SceneInteraction::CameraEndpoint);
        const auto We = path::eval_contrb_direction(scene, spE, {}, vE.w_rev, {}, TransDir::EL, true);
        C = We * vE.alpha;
    }
    else {
        const auto& vL = subpathL.vs[s-1];
        const auto& vE = subpathE.vs[t-1];
        if (!scene->visible(vL.sp, vE.sp)) {
            return {};
        }
        const auto fsL = path::eval_contrb_direction(scene, vL.sp, vL.w_rev, direction(&vL, &vE), vL.comp, TransDir::LE, true);
        const auto fsE = path::eval_contrb_direction(scene, vE.sp, vE.w_rev, direction(&vE, &vL), vE.comp, TransDir::EL, true);
        const auto G = surface::geometry_term(vL.sp.geom, vE.sp.geom);
        C = vL.alpha * fsL * G * fsE * vE.alpha;
    }
    if (math::is_zero(C)) {
        return {};
    }

    // Raster position
    Vec2 rp;
    if (t == 0) {
        const auto& vE = subpathL.vs[s-1];
        rp = *path::raster_position(scene, vE.w_fwd);
    }
    else if (t == 1) {
        const auto& vL = subpathL.vs[s-1];
        const auto& vE = subpathE.vs[0];
        rp = *path::raster_position(scene, direction(&vE, &vL));
    }
    else {
        const auto& vE = subpathE.vs[0];
        rp = *path::raster_position(scene, vE.w_fwd);
    }

    return Splat{ C, rp };
}

// Evaluate MIS weight
Float mis_weight_bidir(const Scene* scene, const Path& subpathE, const Path& subpathL, int s, int t) {
    // Create full path
    thread_local Path path;
    path.vs.clear();
    path.vs.insert(path.vs.end(), subpathL.vs.begin(), subpathL.vs.begin() + s);
    path.vs.insert(path.vs.end(), subpathE.vs.rend() - t, subpathE.vs.rend());
    auto& vL0 = path.vs.front();
    auto& vE0 = path.vs.back();
    vL0.sp = vL0.sp.as_type(SceneInteraction::LightEndpoint);
    vE0.sp = vE0.sp.as_type(SceneInteraction::CameraEndpoint);
    for (int i = s; i < s + t; i++) {
        auto& v = path.vs[i];
        std::swap(v.w_rev, v.w_fwd);
        std::swap(v.pdf_fwd, v.pdf_rev);
    }

    // Recompute cached values
    auto* vL = path.vertex_at(s - 1, TransDir::LE);
    auto* vE = path.vertex_at(t - 1, TransDir::EL);
    auto* vL_prev = path.vertex_at(s - 2, TransDir::LE);
    auto* vE_prev = path.vertex_at(t - 2, TransDir::EL);
    if (vL) {
        vL->w_fwd = direction(vL, vE);
    }
    if (vE) {
        vE->w_rev = direction(vE, vL);
    }
    if (vL) {
        if (!vE) {
            vL->pdf_rev = path::is_connectable_endpoint(scene, vL->sp)
                ? path::pdf_position(scene, vL->sp) : 1_f;
        }
        else {
            vL->pdf_rev = surface::convert_pdf_to_area(
                path::pdf_direction(scene, vE->sp, vE->w_fwd, vE->w_rev, vE->comp, false),
                vE->sp.geom, vL->sp.geom);
        }
    }
    if (vE) {
        if (!vL) {
            vE->pdf_fwd = path::is_connectable_endpoint(scene, vE->sp)
                ? path::pdf_position(scene, vE->sp) : 1_f;
        }
        else {
            vE->pdf_fwd = surface::convert_pdf_to_area(
                path::pdf_direction(scene, vL->sp, vL->w_rev, vL->w_fwd, vL->comp, false),
                vL->sp.geom, vE->sp.geom);
        }
    }
    if (vL_prev) {
        if (!vE && !path::is_connectable_endpoint(scene, vL->sp)) {
            vL_prev->pdf_rev = surface::convert_pdf_to_area(
                path::pdf_primary_ray(scene, vL->sp, vL->w_rev, false),
                vL->sp.geom, vL_prev->sp.geom);
        }
        else {
            vL_prev->pdf_rev = surface::convert_pdf_to_area(
                path::pdf_direction(scene, vL->sp, vL->w_fwd, vL->w_rev, vL->comp, false),
                vL->sp.geom, vL_prev->sp.geom);
        }
    }
    if (vE_prev) {
        if (!vL && !path::is_connectable_endpoint(scene, vE->sp)) {
            vE_prev->pdf_fwd = surface::convert_pdf_to_area(
                path::pdf_primary_ray(scene, vE->sp, vE->w_fwd, false),
                vE->sp.geom, vE_prev->sp.geom);
        }
        else {
            vE_prev->pdf_fwd = surface::convert_pdf_to_area(
                path::pdf_direction(scene, vE->sp, vE->w_rev, vE->w_fwd, vE->comp, false),
                vE->sp.geom, vE_prev->sp.geom);
        }
    }

    // Compute MIS weight
    Float sum = 0_f;
    Float r = 1_f;
    for (int i = s; i < s + t; i++) {
        const auto& v = path.vs[i];
        r *= v.pdf_fwd / v.pdf_rev;
        if (path.is_samplable_bidir(scene, i+1)) {
            sum += r;
        }
    }
    r = 1_f;
    for (int i = s - 1; i >= 0; i--) {
        const auto& v = path.vs[i];
        r *= v.pdf_rev / v.pdf_fwd;
        if (path.is_samplable_bidir(scene, i)) {
            sum += r;
        }
    }
    const auto inv_w = 1_f + sum;
    return 1_f / inv_w;
}

}

// ------------------------------------------------------------------------------------------------

// Enable the output of per-strategy film
#define BDPT_PER_STRATEGY_FILM 0

// Optimized BDPT
class Renderer_BDPT_Optimized final : public Renderer {
private:
    Scene* scene_;                                  // Reference to scene asset
    Film* film_;                                    // Reference to film asset for output
    int min_verts_;                                 // Minimum number of path vertices
    int max_verts_;                                 // Maximum number of path vertices
    std::optional<unsigned int> seed_;              // Random seed
    Component::Ptr<scheduler::Scheduler> sched_;    // Scheduler for parallel processing

    #if BDPT_PER_STRATEGY_FILM
    // Index: (k, s)
    // where k = 2,..,max_verts,
    //       s = 0,...,s
    mutable std::vector<std::vector<Ptr<Film>>> strategy_films_;
    std::unordered_map<std::string, Film*> strategy_film_name_map_;
    #endif

public:
    virtual void construct(const Json& prop) override {
        scene_ = json::comp_ref<Scene>(prop, "scene");
        film_ = json::comp_ref<Film>(prop, "output");
        min_verts_ = json::value<int>(prop, "min_verts", 2);
        max_verts_ = json::value<int>(prop, "max_verts");
        seed_ = json::value_or_none<unsigned int>(prop, "seed");
        const auto sched_name = json::value<std::string>(prop, "scheduler");
        sched_ = comp::create<scheduler::Scheduler>(
            "scheduler::spi::" + sched_name, make_loc("scheduler"), prop);
        #if BDPT_PER_STRATEGY_FILM
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
        #endif
    }

    #if BDPT_PER_STRATEGY_FILM
    virtual Component* underlying(const std::string& name) const override {
        return strategy_film_name_map_.at(name);
    }
    #endif

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
            thread_local Path subpathE;
            thread_local Path subpathL;
            sample_subpath(subpathE, rng, scene_, max_verts_, TransDir::EL);
            sample_subpath(subpathL, rng, scene_, max_verts_, TransDir::LE);
            const int nE = (int)(subpathE.vs.size());
            const int nL = (int)(subpathL.vs.size());
            
            // Generate full path and evaluate contribution
            for (int s = 0; s <= nL; s++) {
                for (int t = 0; t <= nE; t++) {
                    const int k = s + t;
                    if (k < min_verts_ || max_verts_ < k) {
                        continue;
                    }

                    // Connect subpaths and evaluate contribution
                    const auto splat = connect_and_eval_contrb(scene_, subpathE, subpathL, s, t);
                    if (!splat) {
                        continue;
                    }

                    // Evaluate MIS weight
                    const auto w = mis_weight_bidir(scene_, subpathE, subpathL, s, t);
                    const auto C = w * splat->C;

                    // Accumulate contribution
                    film_->splat(splat->rp, C);
                    #if BDPT_PER_STRATEGY_FILM
                    auto& strategy_film = strategy_films_[k-2][s];
                    strategy_film->splat(splat->rp, splat->C);
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

LM_COMP_REG_IMPL(Renderer_BDPT_Optimized, "renderer::bdptopt");

LM_NAMESPACE_END(LM_NAMESPACE)
