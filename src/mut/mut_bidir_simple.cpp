/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/mut.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// Simplified bidirectional mutation that always resample the entire path
class Mut_SimpleBidir final : public Mut {
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

public:
    virtual bool check_mutatable(const Path&) const override {
        // Any paths are mutatable
        return true;
    }

    virtual std::optional<Proposal> sample_proposal(Rng& rng, const Path& curr) const override {
        // Current number of vertices
        const int curr_n = curr.num_verts();

        // Choose number of vertices of the proposed path
        const int prop_n = [&]{
            const int N = max_verts_ - min_verts_ + 1;
            const int prop_n = min_verts_ + glm::clamp(int(rng.u() * N), 0, N - 1);
            return prop_n;
        }();

        // Number of vertices to be deleted and added
        // Simplified version always deletes all vertices
        const int kd = curr_n;
        const int ka = prop_n;

        // Compute number of vertices added from each endpoint
        const int aL = glm::clamp((int)(rng.u() * (ka + 1)), 0, ka);
        const int aE = ka - aL;

        // Sample subpaths
        const auto subpathL = path::sample_subpath(rng, scene_, aL, TransDir::LE);
        if (subpathL.num_verts() != aL) {
            return {};
        }
        const auto subpathE = path::sample_subpath(rng, scene_, aE, TransDir::EL);
        if (subpathE.num_verts() != aE) {
            return {};
        }

        // Generate proposal
        const auto prop_path = path::connect_subpaths(scene_, subpathL, subpathE, aL, aE);
        if (!prop_path) {
            return {};
        }

        // Reject paths with zero-contribution
        if (math::is_zero(prop_path->eval_measurement_contrb_bidir(scene_, aL))) {
            return {};
        }

        return Proposal{
            *prop_path,
            { kd, ka, 0 }
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
        const auto [kd, ka, _] = subspace;
        assert(kd == x_n);
        assert(ka == y_n);

        // pA1: PDF for selecting number of vertices
        const auto pA1 = 1_f / (max_verts_ - min_verts_ + 1);

        // pA2: PDF for selecting the number of vertices to be added for each endpoint
        const auto pA2 = 1_f / (ka + 1);

        // Compute marginal
        Float sum = 0_f;
        for (int i = 0; i <= ka; i++) {
            const auto f = y.eval_measurement_contrb_bidir(scene_, i);
            if (math::is_zero(f)) {
                continue;
            }
            const auto p = y.pdf_bidir(scene_, i);
            if (p == 0_f) {
                continue;
            }
            const auto C = f / p;
            sum += 1_f / path::scalar_contrb(C);
        }

        return pA1 * pA2 * sum;
    }
};

LM_COMP_REG_IMPL(Mut_SimpleBidir, "mut::simple_bidir");

LM_NAMESPACE_END(LM_NAMESPACE)
