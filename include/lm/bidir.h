/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include <lm/path.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup path
    @{
*/

/*!
    \brief Path vertex.

    \rst
    Represents a path vertex of the light transport path.
    \endrst
 */
struct Vert {
    SceneInteraction sp;    //!< Surface interaction.
    int comp;               //!< Component index.
};

/*!
    \brief Light transport path.

    \rst
    Represents a light transport path (or just *path*). 
    This structure can represents two types of paths: *subpath* or *fullpath*. 
    For detail, please refer to :ref:`path_sampling_light_transport_path`.
    \endrst
*/
struct Path {
    //! Path vertices.
    std::vector<Vert> vs;

    // --------------------------------------------------------------------------------------------

    //! Get number of vertices of the path.
    int num_verts() const {
        return (int)(vs.size());
    }

    //! Get path length.
    int num_edges() const {
        return num_verts() - 1;
    }

    /*!
        \brief Compute index to access vertex array according to the transport direction.
        \param i Index of the subpath.
        \param trans_dir Transport direction.

        \rst
        This function returns the index of the path vertex vector ``vs``
        counted from the endpoint of the path according to the transport direction.
        If the transport direction is ``EL``, the function returns the index of i-th vertex
        counted from the light vertex.
        If the transport direction is ``LE``, the function returns the index of i-th vertex
        counted from the eye vertex.
        This function is only valid when the path is a full path.
        \endrst
    */
    int index(int i, TransDir trans_dir) const {
        return trans_dir == TransDir::LE ? i : num_verts() - 1 - i;
    }

    /*!
        \brief Get a pointer to a path vertex (for fullpath).
        \param i Index of the subpath.
        \param trans_dir Transport direction.

        \rst
        This function returns a pointer to a path vertex according to an index of the subpath
        specified by the transport direction.
        If index is out of range, the function returns nullptr.
        This function is only valid when the path is a fullpath.
        \endrst
    */
    const Vert* vertex_at(int i, TransDir trans_dir) const {
        return (0 <= i && i < num_verts()) ? &vs[index(i, trans_dir)] : nullptr;
    }

    /*!
        \brief Get a pointer to a path vertex (for subpath).
        \param i Index of the subpath.
        
        \rst
        This function returns a point to a path vertex inside a subpath.
        If the index is out of range, the function returns nullptr.
        This is only valid the path is a subpath.
        \endrst
    */
    const Vert* subpath_vertex_at(int i) const {
        return (0 <= i && i < num_verts()) ? &vs[i] : nullptr;
    }

    /*!
        \brief Get a pointer to a path vertex (for subpath).
        \param i Index of the subpath.

        \rst
        This function returns a point to a path vertex inside a subpath.
        If the index is out of range, the function returns nullptr.
        This is only valid the path is a subpath.
        \endrst
    */
    Vert* subpath_vertex_at(int i) {
        return (0 <= i && i < num_verts()) ? &vs[i] : nullptr;
    }

    /*!
        \brief Compute direction between path vertices.
        \param v_from Vertex as an origin.
        \param v_to Vertex as a target.
        
        \rst
        This function computes the direction from the vertex ``v_from`` to ``v_to``.
        If either of the vertex is nullptr, the function returns invalid vector.
        The function is useful when the vertex can be empty,
        as a result of out-of-range access by :cpp:func:`lm::Path::vertex_at` or
        :cpp:func:`lm::Path::subpath_vertex_at` functions.
        \endrst
    */
    Vec3 direction(const Vert* v_from, const Vert* v_to) const {
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

    // --------------------------------------------------------------------------------------------

    /*!
        \brief Compute raster position.
        \param scene Scene.

        \rst
        This function computes the raster position using the primary ray from the eye vertex.
        This function is only valid when the path is a fullpath.
        \endrst
    */
    Vec2 raster_position(const Scene* scene) const {
        const auto* vE      = vertex_at(0, TransDir::EL);
        const auto* vE_next = vertex_at(1, TransDir::EL);
        return *path::raster_position(scene, direction(vE, vE_next));
    }

    /*!
        \brief Check if the path is samplable.
        \param scene Scene.
        \param s Strategy index.
        \return True if the path is samplable by the strategy.

        \rst
        This function checks if the path is samplable by the bidirectional path sampling strategy
        indexed by ``s``. This function is necessary to compute bidirecitonal path PDF.
        For detail, please refer to :ref:`path_sampling_connecting_subpaths`.
        \endrst
    */
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

    /*!
        \brief Evaluate subpath contribution.
        \param scene Scene.
        \param l Number of vertices.
        \param trans_dir Transport direction.

        \rst
        This function computes the subpath contribution.
        :math:`\alpha_L(\bar{y})` when ``trans_dir`` is LE.
        :math:`\alpha_E(\bar{z})` when ``trans_dir`` is EL.
        For detail, please refer to :ref:`path_sampling_evaluating_sampling_weight`.
        \endrst
    */
    Vec3 eval_subpath_sampling_weight(const Scene* scene, int l, TransDir trans_dir) const {
        if (l == 0) {
            return Vec3(1_f);
        }

        int i = 0;
        Vec3 alpha(0_f);
        const auto v0 = vertex_at(0, trans_dir);
        if (path::is_connectable_endpoint(scene, v0->sp)) {
            const auto pA = path::pdf_position(scene, v0->sp);
            const auto p_comp = path::pdf_component(scene, v0->sp, v0->comp);
            alpha = Vec3(1_f) / (pA * p_comp);
        }
        else {
            assert(l != 1);
            const auto v1 = vertex_at(1, trans_dir);
            const auto d01 = direction(v0, v1);
            const auto f = path::eval_contrb_direction(scene, v0->sp, {}, d01, v0->comp, trans_dir, false);
            if (math::is_zero(f)) {
                return Vec3(0_f);
            }
            const auto p_comp_v0 = path::pdf_component(scene, v0->sp, v0->comp);
            const auto p_comp_v1 = path::pdf_component(scene, v1->sp, v1->comp);
            const auto p_ray = path::pdf_primary_ray(scene, v0->sp, d01, false);
            alpha = f / (p_ray * p_comp_v0 * p_comp_v1);
            i++;
        }

        for (; i < l - 1; i++) {            
            const auto* v      = vertex_at(i,   trans_dir);
            const auto* v_prev = vertex_at(i-1, trans_dir);
            const auto* v_next = vertex_at(i+1, trans_dir);
            const auto wi = direction(v, v_prev);
            const auto wo = direction(v, v_next);
            const auto f = path::eval_contrb_direction(scene, v->sp, wi, wo, v->comp, trans_dir, false);
            if (math::is_zero(f)) {
                return Vec3(0_f);
            }
            const auto p_comp = path::pdf_component(scene, v_next->sp, v_next->comp);
            const auto p_projSA = path::pdf_direction(scene, v->sp, wi, wo, v->comp, false);
            alpha *= f / p_projSA / p_comp;
        }
        return alpha;
    }

    /*!
        \brief Evaluate connection term.
        \param scene Scene.
        \param s Strategy index.

        \rst
        This function evaluates the connection term :math:`c_{s,t}`.
        For detail, please refer to :ref:`path_sampling_measurement_contribution`.
        \endrst
    */
    Vec3 eval_connection_term(const Scene* scene, int s) const {
        const int n = num_verts();
        const int t = n - s;
        Vec3 cst{};
        if (s == 0 && t > 0) {
            const auto* v      = vertex_at(0, TransDir::LE);
            const auto* v_next = vertex_at(1, TransDir::LE);
            cst = path::eval_contrb_direction(scene, v->sp, {}, direction(v, v_next), v->comp, TransDir::LE, true);
        }
        else if (s > 0 && t == 0) {
            const auto* v      = vertex_at(0, TransDir::EL);
            const auto* v_next = vertex_at(1, TransDir::EL);
            cst = path::eval_contrb_direction(scene, v->sp, {}, direction(v, v_next), v->comp, TransDir::EL, true);
        }
        else if (s > 0 && t > 0) {
            const auto* vL      = vertex_at(s-1, TransDir::LE);
            const auto* vL_prev = vertex_at(s-2, TransDir::LE);
            const auto* vE      = vertex_at(t-1, TransDir::EL);
            const auto* vE_prev = vertex_at(t-2, TransDir::EL);
            const auto fsL = path::eval_contrb_direction(scene, vL->sp, direction(vL, vL_prev), direction(vL, vE), vL->comp, TransDir::LE, true);
            const auto fsE = path::eval_contrb_direction(scene, vE->sp, direction(vE, vE_prev), direction(vE, vL), vE->comp, TransDir::EL, true);
            const auto G = surface::geometry_term(vL->sp.geom, vE->sp.geom);
            cst = fsL * G * fsE;
        }
        return cst;
    }

    /*!
        \brief Evaluate sampling weight.
        \param scene Scene.
        \param s Strategy index.

        \rst
        This function evaluates the sampling weight :math:`C^*_{s,t}(\bar{x})`.
        For detail, please refer to :ref:`path::path_sampling_evaluating_sampling_weight`.
        \endrst
    */
    Vec3 eval_sampling_weight_bidir(const Scene* scene, int s) const {
        const int n = num_verts();
        const int t = n - s;

        // Subpath constribution
        const auto alphaL = eval_subpath_sampling_weight(scene, s, TransDir::LE);
        if (math::is_zero(alphaL)) {
            return Vec3(0_f);
        }
        const auto alphaE = eval_subpath_sampling_weight(scene, t, TransDir::EL);
        if (math::is_zero(alphaE)) {
            return Vec3(0_f);
        }

        // Connection term
        const auto cst = eval_connection_term(scene, s);

        return alphaL * cst * alphaE;
    }

    /*!
        \brief Evaluate measreument contribution function.
        \param scene Scene.
        \param s Strategy index.

        \rst
        This function evaluates the measurement contribution function `f_{s,t}(\bar{x})`.
        For detail, please refer to :ref:`path_sampling_measurement_contribution`.
        \endrst
    */
    Vec3 eval_measurement_contrb_bidir(const Scene* scene, int s) const {
        const int n = num_verts();
        const int t = n - s;

        // Compute contribution for subpath (l = s or t)
        const auto eval_contrb_subpath = [&](int l, TransDir trans_dir) -> Vec3 {
            if (l == 0) {
                return Vec3(1_f);
            }
            auto f_prod = Vec3(1_f);
            for (int i = 0; i < l - 1; i++) {
                const auto* v      = vertex_at(i,   trans_dir);
                const auto* v_prev = vertex_at(i-1, trans_dir);
                const auto* v_next = vertex_at(i+1, trans_dir);
                const auto wi = direction(v, v_prev);
                const auto wo = direction(v, v_next);
                f_prod *= path::eval_contrb_direction(scene, v->sp, wi, wo, v->comp, trans_dir, false);
                f_prod *= surface::geometry_term(v->sp.geom, v_next->sp.geom);
            }
            return f_prod;
        };

        // Product of terms
        const auto f_prod_L = eval_contrb_subpath(s, TransDir::LE);
        const auto f_prod_E = eval_contrb_subpath(t, TransDir::EL);

        // Connection term
        const auto cst = eval_connection_term(scene, s);

        return f_prod_L * cst * f_prod_E;
    }

    /*!
        \brief Evaluate bidirectional path PDF.
        \param scene Scene.
        \param s Strategy index.

        \rst
        This function evaluats the bidirectional path PDF :math:`p_{s,t}(\bar{x})`.
        For detail, please refer to :ref:`path_sampling_bidirectional_path_pdf`.
        \endrst
    */
    Float pdf_bidir(const Scene* scene, int s) const {
        const int n = num_verts();
        const int t = n - s;
        
        // If the path is not samplable by the strategy (s,t), return zero.
        if (!is_samplable_bidir(scene, s)) {
            return 0_f;
        }
        
        // Compute a product of local PDFs
        const auto pdf_subpath = [&](int l, TransDir trans_dir) -> Float {
            if (l == 0) {
                return 1_f;
            }

            int i = 0;
            Float p = 0_f;
            const auto* v0 = vertex_at(0, trans_dir);
            if (path::is_connectable_endpoint(scene, v0->sp)) {
                const auto pA = path::pdf_position(scene, v0->sp);
                const auto p_comp = path::pdf_component(scene, v0->sp, v0->comp);
                p = pA * p_comp;
            }
            else {
                const auto* v1 = vertex_at(1, trans_dir);
                const auto d01 = direction(v0, v1);
                const auto p_ray = path::pdf_primary_ray(scene, v0->sp, d01, false);
                const auto p_comp_v0 = path::pdf_component(scene, v0->sp, v0->comp);
                const auto p_comp_v1 = path::pdf_component(scene, v1->sp, v1->comp);
                p = surface::convert_pdf_to_area(p_ray, v0->sp.geom, v1->sp.geom) * p_comp_v0 * p_comp_v1;
                i++;
            }

            for (; i < l - 1; i++) {
                const auto* v      = vertex_at(i,   trans_dir);
                const auto* v_prev = vertex_at(i-1, trans_dir);
                const auto* v_next = vertex_at(i+1, trans_dir);
                const auto wi = direction(v, v_prev);
                const auto wo = direction(v, v_next);
                const auto p_comp = path::pdf_component(scene, v_next->sp, v_next->comp);
                const auto p_projSA = path::pdf_direction(scene, v->sp, wi, wo, v->comp, false);
                p *= (p_comp * surface::convert_pdf_to_area(p_projSA, v->sp.geom, v_next->sp.geom));
            }
            return p;
        };

        // Compute product of local PDFs for each subpath
        const auto pL = pdf_subpath(s, TransDir::LE);
        const auto pE = pdf_subpath(t, TransDir::EL);

        return pL * pE;
    }

    /*!
        \brief Evaluate MIS weight.
        \param scene Scene.
        \param s Strategy index.

        \rst
        This function evaluates the MIS weight via power heuristic.
        For detail, please refer to :ref:`path_sampling_mis_weight`.
        \endrst
    */
    Float eval_mis_weight(const Scene* scene, int s) const {
        const int n = num_verts();
        const int t = n - s;

        const auto ps = pdf_bidir(scene, s);
        assert(ps > 0_f);

        Float inv_w = 0_f;
        for (int s2 = 0; s2 <= n; s2++) {
            const auto pi = pdf_bidir(scene, s2);
            if (pi == 0_f) {
                continue;
            }
            const auto r = pi / ps;
            inv_w += r*r;
        }

        return 1_f / inv_w;
    }
};

/*!
    @}
*/

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(path)

/*!
    \addtogroup path
    @{
*/

/*!
    \brief Sample path vertices from the endpoint.
    \param rng Random number generator.
    \param path Subpath to be updated.
    \param scene Scene.
    \param max_verts Maximum number of vertices.
    \param trans_dir Transport direction.

    \rst
    This function samples a subpath from the last vertex of the given ``path``.
    For detail, please refer to :ref:`path_sampling_sampling_subpath`.
    \endrst
*/
static void sample_subpath_from_endpoint(Rng& rng, Path& path, const Scene* scene, int max_verts, TransDir trans_dir) {
    // If the requested number of vertices are zero, return immediately.
    // Otherwise the initial vertex would be sampled.
    if (max_verts == 0) {
        return;
    }

    // Perform random walk
    while (path.num_verts() < max_verts) {
        Ray ray;
        if (path.num_verts() == 0) {
            // Sample primary ray
            const auto s = path::sample_primary_ray(rng, scene, trans_dir);
            if (!s) {
                return;
            }
            path.vs.push_back({ s->sp, 0 });
            ray = { s->sp.geom.p, s->wo };
        }
        else {
            // Sample direction
            const int i = path.num_verts() - 1;
            auto* v_curr = path.subpath_vertex_at(i);
            auto* v_prev = path.subpath_vertex_at(i - 1);
            const auto wi = path.direction(v_curr, v_prev);
            const auto s = path::sample_direction(rng, scene, v_curr->sp, wi, v_curr->comp, trans_dir);
            if (!s) {
                break;
            }
            ray = { v_curr->sp.geom.p, s->wo };
        }

        // Intersection to next surface
        const auto hit = scene->intersect(ray);
        if (!hit) {
            break;
        }

        // Sample component
        const auto s_comp = path::sample_component(rng, scene, *hit);

        // Add a vertex
        path.vs.push_back({ *hit, s_comp.comp });

        // Termination
        if (hit->geom.infinite) {
            break;
        }
    }
}

/*!
    \brief Sample a subpath.
    \param rng Random number generator.
    \param scene Scene.
    \param max_verts Maximum number of vertices.
    \param trans_dir Transport direction.
    \return Sampled subpath.

    \rst
    This function samples a subpath.
    For detail, please refer to :ref:`path_sampling_sampling_subpath`.
    \endrst
*/
static Path sample_subpath(Rng& rng, const Scene* scene, int max_verts, TransDir trans_dir) {
    Path path;
    sample_subpath_from_endpoint(rng, path, scene, max_verts, trans_dir);
    return path;
}

/*!
    \brief Connect two subapths and generate a full path.
    \param scene Scene.
    \param subpathL Light subpath.
    \param subpathE Eye subpath.
    \param s Number of light subpath vertices to be used.
    \param t Number of eye subpath vertices to be used.
    \return Connected fullpath. nullopt if failed.

    \rst
    This function takes light and subapth and connect them to compose a fullpath by the specified
    number of vertices from the endpoints of each subpath.
    The function returns nullopt if the connection fails.
    For detail, please refer to :ref:`path_sampling_connecting_subpaths`.
    \endrst
*/
static std::optional<Path> connect_subpaths(const Scene* scene, const Path& subpathL, const Path& subpathE, int s, int t) {
    assert(s >= 0 && t >= 0);
    assert(!(s == 0 && t == 0));

    // Connect two subpaths
    // Returns nullopt if the subpaths are not connectable.
    Path path;
    if (s == 0) {
        if (subpathE.subpath_vertex_at(t-1)->sp.geom.degenerated) {
            return {};
        }
        // Reverse and copy subpathE 
        path.vs.insert(path.vs.end(), subpathE.vs.rend() - t, subpathE.vs.rend());
    }
    else if (t == 0) {
        if (subpathL.subpath_vertex_at(s-1)->sp.geom.degenerated) {
            return {};
        }
        // Copy subpathL as it is
        path.vs.insert(path.vs.end(), subpathL.vs.begin(), subpathL.vs.begin() + s);
    }
    else {
        const auto& vL = subpathL.vs[s - 1];
        const auto& vE = subpathE.vs[t - 1];
        if (vL.sp.geom.infinite || vE.sp.geom.infinite) {
            return {};
        }
        if (!scene->visible(vL.sp, vE.sp)) {
            return {};
        }
        path.vs.insert(path.vs.end(), subpathL.vs.begin(), subpathL.vs.begin() + s);
        path.vs.insert(path.vs.end(), subpathE.vs.rend() - t, subpathE.vs.rend());
    }

    // Check endpoint types
    // We assume the initial vertex of eye subpath is always camera endpoint.
    // That is scene.is_camera(vE) is always true.
    auto& vL = path.vs.front();
    if (!scene->is_light(vL.sp)) {
        return {};
    }
    auto& vE = path.vs.back();
    if (!scene->is_camera(vE.sp)) {
        return {};
    }

    // Update the endpoint types
    vL.sp = vL.sp.as_type(SceneInteraction::LightEndpoint);
    vE.sp = vE.sp.as_type(SceneInteraction::CameraEndpoint);

    return path;
}

/*!
    @}
*/

LM_NAMESPACE_END(path)

LM_NAMESPACE_END(LM_NAMESPACE)
