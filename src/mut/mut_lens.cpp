/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/mut.h>

// Poll failed connections
#define MUT_LENS_POLL_FAILED_CONNECTION 0

#if MUT_LENS_POLL_FAILED_CONNECTION
#include <lm/debug.h>
#include <lm/parallel.h>
#endif

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// Lens perturbation
class Mut_Lens final : public Mut {
private:
    Scene* scene_;
    Float s1_;      // Lower bound of the mutation range
    Float s2_;      // Upper bound of the mutation range

public:
    virtual void construct(const Json& prop) override {
        scene_ = json::comp_ref<Scene>(prop, "scene");
        s1_ = json::value<Float>(prop, "s1");
        s2_ = json::value<Float>(prop, "s2");
    }

private:
    // Perturb a direction using reciprocal distribution
    Vec3 perturb_direction_reciprocal(Rng& rng, Vec3 wo) const {
        // Consider local coordinates around the base direction
        const auto theta = s2_ * std::exp(-std::log(s2_ / s1_) * rng.u());
        const auto phi = 2_f * Pi * rng.u();
        const auto [u, v] = math::orthonormal_basis(wo);
        const auto to_world = Mat3(u, v, wo);
        const auto perturbed_wo = to_world * math::spherical_to_cartesian(theta, phi);
        return perturbed_wo;
    }

    // Find the index of first non-S vertex from camera
    int find_first_non_s(const Path& path) const {
        const int n = path.num_verts();
        int i = 1;
        while (i < n && path.vertex_at(i, TransDir::EL)->specular) {
            i++;
        }
        return i;
    }

public:
    virtual bool check_mutatable(const Path& curr) const override {
        const int n = curr.num_verts();

        // Find first non-S vertex
        const int i = find_first_non_s(curr);
        
        // Path is not mutable if non-S vertex is found on midpoints
        // and the next vertex is S.
        if (i+1 < n && curr.vertex_at(i+1, TransDir::EL)->specular) {
            return false;
        }
        
        // Oherwise the path is mutatable.
        // Example of the mutatable paths: ESSDL, ESSL.
        return true;
    }

    virtual std::optional<Proposal> sample_proposal(Rng& rng, const Path& curr) const override {
        // Number of vertices in the current path
        const int curr_n = curr.num_verts();

        // Check if the path is mutatble with this strategy
        if (!check_mutatable(curr)) {
            return {};
        }

        // Find first non-S vertex from camera
        const int first_non_s_ind = find_first_non_s(curr);
        
        // Perturb eye subpath
        const auto subpathE = [&]() -> std::optional<Path> {
            Path subpathE;
            subpathE.vs.push_back(*curr.vertex_at(0, TransDir::EL));

            // Perturb primary ray direction
            const auto prop_primary_wo = [&] {
                const auto wo = curr.direction(
                    curr.vertex_at(0, TransDir::EL), curr.vertex_at(1, TransDir::EL));
                const auto prop_wo = perturb_direction_reciprocal(rng, wo);
                return prop_wo;
            }();

            // Trace rays until it hit with non-S surface 
            // with the same number of non-S vertices as the current path.
            Vec3 wo = prop_primary_wo;
            for (int i = 1; i <= first_non_s_ind; i++) {
                // Current vertex
                const auto* v = subpathE.subpath_vertex_at(i - 1);

                // Intersection to the next surface
                const auto hit = scene_->intersect({ v->sp.geom.p, wo });
                if (!hit) {
                    // Rejected due to the change of path length
                    return {};
                }

                // Sample next direction
                const auto s = path::sample_direction(rng.next<path::DirectionSampleU>(), scene_, *hit, -wo, TransDir::EL);
                if (!s) {
                    return {};
                }

                // Terminate if it finds non-S vertex at i < first_non_s_ind
                if (i < first_non_s_ind && !s->specular) {
                    return {};
                }

                // Terminate if it finds S vertex at i = first_non_s_ind
                if (i == first_non_s_ind && s->specular) {
                    return {};
                }

                // Add a vertex
                subpathE.vs.push_back({ *hit, s->specular });
                wo = s->wo;
            }

            return subpathE;
        }();
        if (!subpathE) {
            return {};
        }

        // Number of vertices in each subpath
        const int nE = subpathE->num_verts();
        const int nL = curr_n - nE;
        assert(nE == first_non_s_ind + 1);

        // Light subpath
        Path subpathL;
        for (int i = 0; i < nL; i++) {
            subpathL.vs.push_back(*curr.vertex_at(i, TransDir::LE));
        }

        // Generate proposal
        const auto prop_path = path::connect_subpaths(scene_, subpathL, *subpathE, nL, nE);
        if (!prop_path) {
            #if MUT_LENS_POLL_FAILED_CONNECTION
            if (nL > 0 && nE > 0 && parallel::main_thread()) {
                debug::poll({
                    {"id", "mut_lens_failed_connection"},
                    {"v1", subpathL.vs[nL-1].sp.geom.p},
                    {"v2", subpathE->vs[nE-1].sp.geom.p}
                });
            }
            #endif
            return {};
        }

        // Reject paths with zero-contribution
        if (math::is_zero(prop_path->eval_measurement_contrb_bidir(scene_, nL))) {
            return {};
        }

        return Proposal{*prop_path, {}};
    }

    virtual Subspace reverse_subspace(const Subspace& subspace) const override {
        return subspace;
    }
    
    virtual Float eval_Q(const Path&, const Path& y, const Subspace&) const override {
        // Number of vertices in each path
        const int x_n = y.num_verts();
        const int y_n = y.num_verts();
        assert(x_n == y_n);
        const int n = x_n;

        // Find first non-S vertex from camera
        const int first_non_s_ind = find_first_non_s(y);
        const int nE = first_non_s_ind + 1;
        const int nL = n - nE;

        // Evaluate terms
        // The most of terms are cancelled out.
        // Eventualy we only need to consider alpha_t * c_{s,t}.
        const auto alpha = y.eval_subpath_contrb(scene_, nE, TransDir::EL);
        assert(!math::is_zero(alpha));
        const auto cst = y.eval_connection_term(scene_, nL);
        if (math::is_zero(cst)) {
            return 0_f;
        }

        return 1_f / path::scalar_contrb(alpha * cst);
    }
};

LM_COMP_REG_IMPL(Mut_Lens, "mut::lens");

LM_NAMESPACE_END(LM_NAMESPACE)
