/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/mut.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

#define MUT_BIDIR_SIMPLIFY_DELETE_ALL 0
#define MUT_BIDIR_SIMPLIFY_DELETE_ALL_BUT_ONE 0
#define MUT_BIDIR_SIMPIFY_UNIFORM_DELETED_RANGE 0
#define MUT_BIDIR_SIMPLIFY_PT 0

// Bidirectional mutation
class Mut_Bidir final : public Mut {
private:
    Scene* scene_;
    int min_verts_;
    int max_verts_;

public:
    virtual void construct(const Json& prop) override {
        scene_ = json::comp_ref<Scene>(prop, "scene");
        min_verts_ = json::value<int>(prop, "min_verts", 2);
        max_verts_ = json::value<int>(prop, "max_verts");
    }

private:
    // Create a distribution for selecting deleted range
    std::optional<Dist> create_delete_range_dist(const Path& x, int kd) const {
        Dist dist;
        const int xn = x.num_verts();
        for (int i = 0; i <= xn - kd; i++) {
            const int j = i + kd - 1;
            const bool not_selectable =
                (i > 0 && x.vs[i - 1].specular) || (j < xn - 1 && x.vs[j + 1].specular);
            dist.add(not_selectable ? 0_f : 1_f);
        }
        if (dist.c.back() == 0_f) {
            return {};
        }
        dist.norm();
        return dist;
    }

public:
    virtual bool check_mutatable(const Path&) const override {
        // Any paths are mutatable
        return true;
    }

    virtual std::optional<Proposal> sample_proposal(Rng& rng, const Path& curr) const override {
        // Current number of vertices
        const int curr_n = curr.num_verts();

        // Choose number of vertices of the proposed path
        const int prop_n = [&] {
            const int N = max_verts_ - min_verts_ + 1;
            const int prop_n = min_verts_ + glm::clamp(int(rng.u() * N), 0, N - 1);
            return prop_n;
        }();

        // Choose number of vertices to be deleted and added
        #if MUT_BIDIR_SIMPLIFY_DELETE_ALL
        const int kd = curr_n;
        #elif MUT_BIDIR_SIMPLIFY_DELETE_ALL_BUT_ONE
        const int kd = curr_n - 1;
        #else
        const int kd = [&] {
            // We constraint the minimum range of kd to 1, which can avoid some tricky corner cases.
            // Note that we also need to constraint ka >= 1,
            // otherwise proposal cannot mutate back to the original state.
            const int min = std::max(1, curr_n - prop_n + 1);
            const int max = curr_n;
            const int N = max - min + 1;
            const int kd = min + glm::clamp(int(rng.u() * N), 0, N - 1);
            return kd;
        }();
        #endif
        const int ka = prop_n - curr_n + kd;
        
        // Choose the range of deleted vertices [dL, dM]
        #if MUT_BIDIR_SIMPIFY_UNIFORM_DELETED_RANGE
        // Note: This only works for the scene with non-specular materials.
        const int dL = glm::clamp((int)(rng.u() * (curr_n - kd + 1)), 0, curr_n - kd);
        #else
        const auto delete_range_dist = create_delete_range_dist(curr, kd);
        if (!delete_range_dist) {
            return {};
        }
        const int dL = delete_range_dist->sample(rng.u());
        #endif
        const int dE = dL + kd - 1;

        // Compute number of vertices added from each endpoint
        #if MUT_BIDIR_SIMPLIFY_PT
        const int aL = 0;
        const int aE = ka;
        #else
        const int aL = glm::clamp((int)(rng.u() * (ka + 1)), 0, ka);
        const int aE = ka - aL;
        #endif

        // Sample light subpath
        Path subpathL;
        const int nL = dL + aL;
        for (int i = 0; i < dL; i++) {
            subpathL.vs.push_back(*curr.vertex_at(i, TransDir::LE));
        }
        path::sample_subpath_from_endpoint(rng, subpathL, scene_, nL, TransDir::LE);
        if (subpathL.num_verts() != nL) {
            return {};
        }

        // Sample eye subpath
        Path subpathE;
        const int nE = curr_n - dE + aE - 1;
        for (int i = 0; i <= curr_n-dE-2; i++) {
            subpathE.vs.push_back(*curr.vertex_at(i, TransDir::EL));
        }
        path::sample_subpath_from_endpoint(rng, subpathE, scene_, nE, TransDir::EL);
        if (subpathE.num_verts() != nE) {
            return {};
        }
        
        // Generate proposal
        const auto prop_path = path::connect_subpaths(scene_, subpathL, subpathE, nL, nE);
        if (!prop_path) {
            return {};
        }

        // Reject paths with zero-contribution
        if (math::is_zero(prop_path->eval_measurement_contrb_bidir(scene_, nL))) {
            return {};
        }

        return Proposal{
            *prop_path,
            { kd, ka, dL }
        };
    }

    virtual Subspace reverse_subspace(const Subspace& subspace) const override {
        const auto [kd, ka, dL] = subspace;
        return { ka, kd, dL };
    }

    virtual Float eval_Q(const Path& x, const Path& y, const Subspace& subspace) const override {
        // Number of vertices in each path
        const int x_n = x.num_verts();
        const int y_n = y.num_verts();

        // Selected subspace
        const auto [kd, ka, dL] = subspace;
        assert(x_n - kd + ka == y_n);

        // pA1: PDF for selecting number of vertices
        const auto pA1 = 1_f / (max_verts_ - min_verts_ + 1);

        // pA2: PDF for selecting the number of vertices to be added for each endpoint
        const auto pA2 = 1_f / (ka + 1);

        // pD1: PDF for selecting the number of vertices to be deleted
        #if MUT_BIDIR_SIMPLIFY_DELETE_ALL || MUT_BIDIR_SIMPLIFY_DELETE_ALL_BUT_ONE 
        const auto pD1 = 1_f;
        #else
        const auto pD1 = [&] {
            const int min = std::max(1, x_n - y_n + 1);
            const int max = x_n;
            const int N = max - min + 1;
            return 1_f / N;
        }();
        #endif

        // pD2: PDF for selecting the range of the deleted vertices
        #if MUT_BIDIR_SIMPIFY_UNIFORM_DELETED_RANGE
        const auto pD2 = 1_f / (x_n - kd + 1);
        #else
        const auto delete_range_dist = create_delete_range_dist(x, kd);
        if (!delete_range_dist) {
            return 0_f;
        }
        const auto pD2 = delete_range_dist->pmf(dL);
        #endif

        // Compute marginal
        Float sum = 0_f;
        #if MUT_BIDIR_SIMPLIFY_PT
        for (int i = 0; i <= 0; i++) {
        #else
        for (int i = 0; i <= ka; i++) {
        #endif
            const auto f = y.eval_measurement_contrb_bidir(scene_, dL+i);
            if (math::is_zero(f)) {
                continue;
            }
            const auto p = y.pdf_bidir(scene_, dL+i);
            if (p == 0_f) {
                continue;
            }
            const auto C = f / p;
            sum += 1_f / path::scalar_contrb(C);
        }

        return pA1 * pA2 * pD1 * pD2 * sum;
    }
};

LM_COMP_REG_IMPL(Mut_Bidir, "mut::bidir");

LM_NAMESPACE_END(LM_NAMESPACE)
