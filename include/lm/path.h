/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "scene.h"
#include "camera.h"
#include "light.h"
#include "material.h"
#include "medium.h"
#include "phase.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(path)

/*!
    \addtogroup path
    @{
*/

#pragma region Ray sampling

//! Result of ray sampling.
struct RaySample {
    SceneInteraction sp;    //!< Sampled scene interaction.
    Vec3 wo;                //!< Sampled direction.
    Vec3 weight;            //!< Contribution divided by probability.
    bool specular;          //!< Sampled from specular distribution.

    /*!
        \brief Get a ray from the sample.
        
        \rst
        This function constructs a :cpp:class:`lm::Ray`
        structure from the ray sample.
        \endrst
    */
    Ray ray() const {
        return { sp.geom.p, wo };
    }
};

/*!
    \brief Generate a primary ray.
    \param scene Scene.
    \param rp Raster position in [0,1]^2.
    \return Generated primary ray.

    \rst
    This function deterministically generates a primary ray
    corresponding to the given raster position.
    \endrst
*/
static Ray primary_ray(const Scene* scene, Vec2 rp) {
    const auto* camera = scene->node_at(scene->camera_node()).primitive.camera;
    return camera->primary_ray(rp);
}

//! Random number input for ray sampling.
struct RaySampleU {
    Vec2 up;    // For position
    Vec2 upc;   // For positional component
    Vec2 ud;    // For direction
    Vec2 udc;   // For directional component
};

/*!
    \brief Primary ray sampling.
    \param u Random number input.
    \param scene Scene.
    \param trans_dir Transport direction.

    \rst
    This function samples a primary ray according the transport direction.
    If the transport direction is ``LE``, the function generates a primary ray from a light.
    If the transport direction is ``EL``, the function generates a primary ray from a camera.
    In both cases, this function returns ``nullopt`` if the sampling failed,
    or the case when the early return is possible for instance when 
    the evaluated contribution of the sampled direction is zero.
    For detail, please refer to :ref:`path_sampling_primary_ray_sampling`.
    \endrst
*/
static std::optional<RaySample> sample_primary_ray(const RaySampleU& u, const Scene* scene, TransDir trans_dir) {
    if (trans_dir == TransDir::EL) {
        const auto* camera = scene->node_at(scene->camera_node()).primitive.camera;
        const auto s = camera->sample_ray({u.ud});
        if (!s) {
            return {};
        }
        return RaySample{
            SceneInteraction::make_camera_endpoint(
                scene->camera_node(),
                s->geom
            ),
            s->wo,
            s->weight
        };
    }
    else if (trans_dir == TransDir::LE) {
        const auto [light_index, p_sel] = scene->sample_light_selection(u.upc[0]);
        const auto light_primitive_index = scene->light_primitive_index_at(light_index);
        const auto& node = scene->node_at(light_primitive_index.index);
        const auto* light = node.primitive.light;
        const auto s = light->sample_ray({u.up,u.upc[1],u.ud}, light_primitive_index.global_transform);
        if (!s) {
            return {};
        }
        return RaySample{
            SceneInteraction::make_light_endpoint(
                light_primitive_index.index,
                s->geom
            ),
            s->wo,
            s->weight
        };
    }
    LM_UNREACHABLE_RETURN();
}

/*!
    \brief Primary ray sampling.
    \param rng Random number generator.
    \param scene Scene.
    \param trans_dir Transport direction.

    \rst
    This function samples a primary ray according the transport direction.
    This version takes random number generator instead of arbitrary random numbers.
    \endrst
*/
static std::optional<RaySample> sample_primary_ray(Rng& rng, const Scene* scene, TransDir trans_dir) {
    return sample_primary_ray(rng.next<RaySampleU>(), scene, trans_dir);
}

/*!
    \brief Evaluate PDF for primary ray sampling.
    \param scene Scene.
    \param sp Sampled scene interaction.
    \param wo Sampled outgoing direction.
    \param eval_delta If true, evaluate delta function.

    \rst
    This function evaluate the PDF correcponding to :cpp:func:`sample_primary_ray` function.
    For detail, please refer to :ref:`path_sampling_primary_ray_sampling`.
    \endrst
*/
static Float pdf_primary_ray(const Scene* scene, const SceneInteraction& sp, Vec3 wo, bool eval_delta) {
    const auto& primitive = scene->node_at(sp.primitive).primitive;
    if (sp.is_type(SceneInteraction::CameraEndpoint)) {
        return primitive.camera->pdf_ray(sp.geom, wo);
    }
    else if (sp.is_type(SceneInteraction::LightEndpoint)) {
        const auto light_index = scene->light_index_at(sp.primitive);
        const auto light_primitive_index = scene->light_primitive_index_at(light_index);
        const auto pL_sel = scene->pdf_light_selection(light_index);
        const auto pL_ray = primitive.light->pdf_ray(sp.geom, wo, light_primitive_index.global_transform, eval_delta);
        return pL_sel * pL_ray;
    }
    LM_UNREACHABLE_RETURN();
}

#pragma endregion

// ------------------------------------------------------------------------------------------------

#pragma region Component sampling

//! Result of component sampling.
struct ComponentSample {
    int comp;
    Float weight;
};

//! Result of component sampling.
struct ComponentSampleU {
    Vec2 uc;
};

/*!
    \brief Component sampling.
    \param u Random number input.
    \param scene Scene.
    \param sp Scene interaction.
    \param wi Incident direction.
    
    \rst
    This function samples a component of the scene interaction accroding to its type.
    For detail, please refer to :ref:`path_sampling_component_sampling`.
    \endrst
*/
static ComponentSample sample_component(const ComponentSampleU& u, const Scene* scene, const SceneInteraction& sp, Vec3 wi) {
    if (sp.is_type(SceneInteraction::SurfaceInteraction)) {
        const auto* material = scene->node_at(sp.primitive).primitive.material;
        const auto s = material->sample_component({u.uc}, sp.geom, wi);
        return { s.comp, s.weight };
    }
    return { 0, 1_f };
}

/*!
    \brief Component sampling.
    \param rng Random number generator.
    \param scene Scene.
    \param sp Scene interaction.

    \rst
    This function samples a component of the scene interaction accroding to its type.
    This version takes random number generator instead of arbitrary random numbers.
    \endrst
*/
static ComponentSample sample_component(Rng& rng, const Scene* scene, const SceneInteraction& sp, Vec3 wi) {
    return sample_component(rng.next<ComponentSampleU>(), scene, sp, wi);
}

/*!
    \brief Evaluate PDF for component sampling.
    \param scene Scene.
    \param sp Scene interaction.
    \param comp Sampled component index.

    \rst
    This function evaluate the PDF correcponding to :cpp:func:`sample_component` function.
    For detail, please refer to :ref:`path_sampling_component_sampling`.
    \endrst
*/
static Float pdf_component(const Scene* scene, const SceneInteraction& sp, Vec3 wi, int comp) {
    if (sp.is_type(SceneInteraction::SurfaceInteraction)) {
        const auto* material = scene->node_at(sp.primitive).primitive.material;
        return material->pdf_component(comp, sp.geom, wi);
    }
    return 1_f;
}

#pragma endregion

// ------------------------------------------------------------------------------------------------

#pragma region Position sampling

//! Result of endpint sampling.
struct PositionSample {
    SceneInteraction sp;
    Vec3 weight;
};

//! Random number input for endpoint sampling.
struct PositionSampleU {
    Vec2 up;
    Vec2 upc;
};

/*!
    \brief Endpoint sampling.
    \param u Random number input.
    \param scene Scene.
    \param trans_dir Transport direction.

    \rst
    This function samples an endpoint either on light or camera.
    If the transport direction is ``LE``, an endpoint is sampled from light.
    If the transport direction is ``EL``, an endpoint is sampled from camera.
    For detail, please refer to :ref:`path_sampling_endpoint_sampling`.
    \endrst
*/
static std::optional<PositionSample> sample_position(const PositionSampleU& u, const Scene* scene, TransDir trans_dir) {
    if (trans_dir == TransDir::EL) {
        const auto* camera = scene->node_at(scene->camera_node()).primitive.camera;
        const auto s = camera->sample_position({ u.up });
        if (!s) {
            return {};
        }
        return PositionSample{
            SceneInteraction::make_camera_endpoint(
                scene->camera_node(),
                s->geom
            ),
            s->weight
        };
    }
    else if (trans_dir == TransDir::LE) {
        const auto [light_index, p_sel] = scene->sample_light_selection(u.upc[0]);
        const auto light_primitive_index = scene->light_primitive_index_at(light_index);
        const auto& node = scene->node_at(light_primitive_index.index);
        const auto* light = node.primitive.light;
        const auto s = light->sample_position({ u.up, u.upc[1] }, light_primitive_index.global_transform);
        if (!s) {
            return {};
        }
        return PositionSample{
            SceneInteraction::make_light_endpoint(
                light_primitive_index.index,
                s->geom
            ),
            s->weight
        };
    }
    LM_UNREACHABLE_RETURN();
}

/*!
    \brief Endpoint sampling.
    \param rng Random number generator.
    \param scene Scene.
    \param trans_dir Transport direction.

    \rst
    This function samples an endpoint either on light or camera.
    This version takes random number generator instead of arbitrary random numbers.
    \endrst
*/
static std::optional<PositionSample> sample_position(Rng& rng, const Scene* scene, TransDir trans_dir) {
    return sample_position(rng.next<PositionSampleU>(), scene, trans_dir);
}

/*!
    \brief Evaluate PDF for endpoint sampling.
    \param scene Scene.
    \param sp Sampled scene interaction.

    \rst
    This function evaluate the PDF correcponding to :cpp:func:`sample_position` function.
    For detail, please refer to :ref:`path_sampling_endpoint_sampling`.
    \endrst
*/
static Float pdf_position(const Scene* scene, const SceneInteraction& sp) {
    if (!sp.is_type(SceneInteraction::Endpoint)) {
        LM_THROW_EXCEPTION(Error::Unsupported,
            "pdf_position() does not support non-endpoint interactions.");
    }
    const auto& primitive = scene->node_at(sp.primitive).primitive;
    if (sp.is_type(SceneInteraction::CameraEndpoint)) {
        return primitive.camera->pdf_position(sp.geom);
    }
    else if (sp.is_type(SceneInteraction::LightEndpoint)) {
        const auto light_index = scene->light_index_at(sp.primitive);
        const auto light_primitive_index = scene->light_primitive_index_at(light_index);
        const auto pL_sel = scene->pdf_light_selection(light_index);
        const auto pL_pos = primitive.light->pdf_position(sp.geom, light_primitive_index.global_transform);
        return pL_sel * pL_pos;
    }
    LM_UNREACHABLE_RETURN();
}

#pragma endregion

// ------------------------------------------------------------------------------------------------

#pragma region Direction sampling

//! Result of direction sampling.
struct DirectionSample {
    Vec3 wo;
    Vec3 weight;
};

//! Random number input for direction sampling.
struct DirectionSampleU {
    Vec2 ud;
    Vec2 udc;
};

/*!
    \brief Direction sampling.
    \param u Random number input. 
    \param scene Scene.
    \param sp Scene interaction.
    \param wi Incident direction.
    \param comp Component index.
    \param trans_dir Transport direction.

    \rst
    This function samples a direction given a scene interaciton point and incident direction from the point.
    For detail, please refer to :ref:`path_sapmling_direction_sampling`.
    \endrst
*/
static std::optional<DirectionSample> sample_direction(const DirectionSampleU& u, const Scene* scene, const SceneInteraction& sp, Vec3 wi, int comp, TransDir trans_dir) {
    const auto& primitive = scene->node_at(sp.primitive).primitive;
    if (sp.is_type(SceneInteraction::CameraEndpoint)) {
        const auto s = primitive.camera->sample_direction({ u.ud }, sp.geom);
        if (!s) {
            return {};
        }
        return DirectionSample{
            s->wo,
            s->weight
        };
    }
    else if (sp.is_type(SceneInteraction::LightEndpoint)) {
        const auto s = primitive.light->sample_direction({ u.ud }, sp.geom);
        if (!s) {
            return {};
        }
        return DirectionSample{
            s->wo,
            s->weight
        };
    }
    else if (sp.is_type(SceneInteraction::MediumInteraction)) {
        const auto s = primitive.medium->phase()->sample_direction({u.ud}, sp.geom, wi);
        if (!s) {
            return {};
        }
        return DirectionSample{
            s->wo,
            s->weight
        };
    }
    else if (sp.is_type(SceneInteraction::SurfaceInteraction)) {
        const auto s = primitive.material->sample_direction({u.ud,u.udc}, sp.geom, wi, comp, static_cast<Material::TransDir>(trans_dir));
        if (!s) {
            return {};
        }
        const auto sn_corr = surface::shading_normal_correction(sp.geom, wi, s->wo, trans_dir);
        return DirectionSample{
            s->wo,
            s->weight * sn_corr
        };
    }
    LM_UNREACHABLE_RETURN();  
};

/*!
    \brief Direction sampling.
    \param rng Random number generator. 
    \param scene Scene.
    \param sp Scene interaction.
    \param wi Incident direction.
    \param comp Component index.
    \param trans_dir Transport direction.

    \rst
    This function samples a direction given a scene interaciton point and incident direction from the point.
    This version takes random number generator instead of arbitrary random numbers.
    \endrst
*/
static std::optional<DirectionSample> sample_direction(Rng& rng, const Scene* scene, const SceneInteraction& sp, Vec3 wi, int comp, TransDir trans_dir) {
    return sample_direction(rng.next<DirectionSampleU>(), scene, sp, wi, comp, trans_dir);
}

/*!
    \brief Evaluate pdf for direction sampling.
    \param scene Scene.
    \param sp Scene interaction.
    \param wi Incident ray direction.
    \param wo Sampled outgoing ray direction.
	\param comp Component index.
    \param eval_delta If true, evaluate delta function.
    \return Evaluated pdf.

    \rst
    This function evaluate the PDF correcponding to :cpp:func:`sample_direction` function.
    For detail, please refer to :ref:`path_sapmling_direction_sampling`.
    \endrst
*/
static Float pdf_direction(const Scene* scene, const SceneInteraction& sp, Vec3 wi, Vec3 wo, int comp, bool eval_delta) {
    const auto& primitive = scene->node_at(sp.primitive).primitive;
    switch (sp.type) {
        case SceneInteraction::CameraEndpoint:
            return primitive.camera->pdf_direction(sp.geom, wo);
        case SceneInteraction::LightEndpoint:
            return primitive.light->pdf_direction(sp.geom, wo);
        case SceneInteraction::MediumInteraction:
            return primitive.medium->phase()->pdf_direction(sp.geom, wi, wo);
        case SceneInteraction::SurfaceInteraction:
            return primitive.material->pdf_direction(sp.geom, wi, wo, comp, eval_delta);
    }
    LM_UNREACHABLE_RETURN();
}

#pragma endregion

// ------------------------------------------------------------------------------------------------

#pragma region Direct endpoint sampling

/*!
    \brief Direct endpoint sampling.
    \param u Random number input.
    \param scene Scene.
    \param sp Scene interaction.
    \param trans_dir Transport direction.

    \rst
    This function samples a ray from the light to the given scene interaction.
    Be careful not to confuse the sampled ray with the ray sampled via :cpp:func:`Scene::sample_ray`
    function from a light source. Both rays are sampled from the different distributions
    and if you want to evaluate densities you want to use different functions.
    For detail, please refer to :ref:`path_sampling_direct_endpoint_sampling`.
    \endrst
*/
static std::optional<RaySample> sample_direct(const RaySampleU& u, const Scene* scene, const SceneInteraction& sp, TransDir trans_dir) {
    if (trans_dir == TransDir::EL) {
        const auto& primitive = scene->node_at(scene->camera_node()).primitive;
        const auto s = primitive.camera->sample_direct({u.ud}, sp.geom);
        if (!s) {
            return {};
        }
        return RaySample{
            SceneInteraction::make_camera_endpoint(
                scene->camera_node(),
                s->geom
            ),
            s->wo,
            s->weight
        };
    }
    else if (trans_dir == TransDir::LE) {
        // Sample a light
        const auto [light_index, p_sel] = scene->sample_light_selection(u.upc[0]);

        // Sample a position on the light
        const auto light_primitive_index = scene->light_primitive_index_at(light_index);
        const auto& primitive = scene->node_at(light_primitive_index.index).primitive;
        const auto s = primitive.light->sample_direct({u.up,u.upc[1],u.ud}, sp.geom, light_primitive_index.global_transform);
        if (!s) {
            return {};
        }
        return RaySample{
            SceneInteraction::make_light_endpoint(
                light_primitive_index.index,
                s->geom
            ),
            s->wo,
            s->weight / p_sel
        };
    }
    LM_UNREACHABLE_RETURN();
}

/*!
    \brief Direct endpoint sampling.
    \param rng Random number generator.
    \param scene Scene.
    \param sp Scene interaction.
    \param trans_dir Transport direction.

    \rst
    This function samples a ray from the light to the given scene interaction.
    This version takes random number generator instead of arbitrary random numbers.
    \endrst
*/
static std::optional<RaySample> sample_direct(Rng& rng, const Scene* scene, const SceneInteraction& sp, TransDir trans_dir) {
    return sample_direct(rng.next<RaySampleU>(), scene, sp, trans_dir);
}

/*!
    \brief Evaluate pdf for endpoint sampling given a scene interaction.
    \param scene Scene.
    \param sp Scene interaction.
    \param sp_endpoint Sampled scene interaction of the endpoint.
    \param wo Sampled outgoing ray directiom *from* the endpoint.
    \param eval_delta If true, evaluate delta function.

    \rst
    This function evaluate pdf for the ray sampled via :cpp:func:`Scene::sample_direct_light`
    or :cpp:func:`Scene::sample_direct_camera`.
    Be careful ``wo`` is the outgoing direction originated from ``sp_endpoint``, not ``sp``.
    For detail, please refer to :ref:`path_sampling_direct_endpoint_sampling`.
    \endrst
*/
static Float pdf_direct(const Scene* scene, const SceneInteraction& sp, const SceneInteraction& sp_endpoint, Vec3 wo, bool eval_delta) {
    if (!sp_endpoint.is_type(SceneInteraction::Endpoint)) {
        LM_THROW_EXCEPTION(Error::Unsupported,
            "pdf_direct() does not support non-endpoint interactions.");
    }

    const auto& primitive = scene->node_at(sp_endpoint.primitive).primitive;
    if (sp_endpoint.is_type(SceneInteraction::CameraEndpoint)) {
        return primitive.camera->pdf_direct(sp.geom, sp_endpoint.geom, wo);
    }
    else if (sp_endpoint.is_type(SceneInteraction::LightEndpoint)) {
        const int light_index = scene->light_index_at(sp_endpoint.primitive);
        const auto light_primitive_index = scene->light_primitive_index_at(light_index);
        const auto pL_sel = scene->pdf_light_selection(light_index);
        const auto pL_pos = primitive.light->pdf_direct(
            sp.geom, sp_endpoint.geom, light_primitive_index.global_transform, wo, eval_delta);
        return pL_sel * pL_pos;
    }

    LM_UNREACHABLE_RETURN();
}

#pragma endregion

// ------------------------------------------------------------------------------------------------

#pragma region Distance sampling

//! Result of distance sampling.
struct DistanceSample {
    SceneInteraction sp;    //!< Sampled interaction point.
    Vec3 weight;            //!< Contribution divided by probability.
};

/*!
    \brief Sample a distance in a ray direction.
    \param rng Random number generator.
    \param scene Scene.
    \param sp Scene interaction.
    \param wo Ray direction.

    \rst
    This function samples either a point in a medium or a point on the surface.
    Note that we don't provide corresponding PDF function because
    some underlying distance sampling technique might not have the analytical representation.
    \endrst
*/
static std::optional<DistanceSample> sample_distance(Rng& rng, const Scene* scene, const SceneInteraction& sp, Vec3 wo) {
    // Intersection to next surface
    const auto hit = scene->intersect({ sp.geom.p, wo }, Eps, Inf);
    const auto dist = hit && !hit->geom.infinite ? glm::length(hit->geom.p - sp.geom.p) : Inf;

    // Sample a distance
    const auto* medium = scene->node_at(scene->medium_node()).primitive.medium;
    const auto ds = medium->sample_distance(rng, { sp.geom.p, wo }, 0_f, dist);
    if (ds && ds->medium) {
        // Medium interaction
        return DistanceSample{
            SceneInteraction::make_medium_interaction(
                scene->medium_node(),
                PointGeometry::make_degenerated(ds->p)
            ),
            ds->weight
        };
    }
    else {
        // Surface interaction
        if (!hit) {
            return {};
        }
        return DistanceSample{
            *hit,
            ds ? ds->weight : Vec3(1_f)
        };
    }
}

/*!
    \brief Evaluate transmittance.
    \param rng Random number generator.
    \param scene Scene.
    \param sp1 Scene interaction of the first point.
    \param sp2 Scene interaction of the second point.

    \rst
    This function evaluates transmittance between two scene interaction events.
    This function might need a random number generator
    because heterogeneous media needs stochastic estimation.
    If the space between ``sp1`` and ``sp2`` is vacuum (i.e., no media),
    this function is conceptually equivalent to :cpp:func:`Scene::visible`.
    \endrst
*/
static Vec3 eval_transmittance(Rng& rng, const Scene* scene, const SceneInteraction& sp1, const SceneInteraction& sp2) {
    if (!scene->visible(sp1, sp2)) {
        return Vec3(0_f);
    }
    const int medium_index = scene->medium_node();
    if (medium_index == -1) {
        return Vec3(1_f);
    }
        
    // Extended distance between two points
    assert(!sp1.geom.infinite);
    const auto dist = !sp2.geom.infinite
        ? glm::distance(sp1.geom.p, sp2.geom.p)
        : Inf;
    const auto wo = !sp2.geom.infinite
        ? glm::normalize(sp2.geom.p - sp1.geom.p)
        : -sp2.geom.wo;

    const auto* medium = scene->node_at(medium_index).primitive.medium;
    return medium->eval_transmittance(rng, { sp1.geom.p, wo }, 0_f, dist);
}

#pragma endregion

// ------------------------------------------------------------------------------------------------

#pragma region Evaluating contribution

/*!
    \brief Check if the scene intersection is specular.
    \param scene Scene.
    \param sp Scene interaction.
    \param comp Component index.
    
    \rst
    This function checks if the directional component :math:`f_{s\Sigma}(\mathbf{x},j,\omega_i,\omega_o)`
    contains a delta function. If contains, this function returns true.
    For detail, please refer to :ref:`path_sampling_specular_vertex`.
    \endrst
*/
static bool is_specular_component(const Scene* scene, const SceneInteraction& sp, int comp) {
    const auto& primitive = scene->node_at(sp.primitive).primitive;
    if (sp.is_type(SceneInteraction::SurfaceInteraction)) {
        return primitive.material->is_specular_component(comp);
    }
    else if (sp.is_type(SceneInteraction::LightEndpoint)) {
        return primitive.light->is_specular();
    }
    return false;
}

/*!
    \brief Check if the endpoint is connectable.
    \param scene Scene.
    \param sp Scene interaction.

    \rst
    This function checks if the endpoint can be connectable from another point in the scene.
    For detail, please refer to :ref:`path_sampling_connectable_endpoint`.
    \endrst
*/
static bool is_connectable_endpoint(const Scene* scene, const SceneInteraction& sp) {
    const auto& primitive = scene->node_at(sp.primitive).primitive;
    if (sp.is_type(SceneInteraction::CameraEndpoint)) {
        return primitive.camera->is_connectable(sp.geom);
    }
    else if (sp.is_type(SceneInteraction::LightEndpoint)) {
        return primitive.light->is_connectable(sp.geom);
    }
    LM_UNREACHABLE_RETURN();
}

/*!
    \brief Compute a raster position.
    \param scene Scene.
    \param wo Primary ray direction.
    \return Raster position.
*/
static std::optional<Vec2> raster_position(const Scene* scene, Vec3 wo) {
    const auto* camera = scene->node_at(scene->camera_node()).primitive.camera;
    return camera->raster_position(wo);
}

/*!
    \brief Evaluate directional components.
    \param scene Scene.
    \param sp Scene interaction.
    \param wi Incident ray direction.
    \param wo Outgoing ray direction.
	\param comp Component index.
    \param trans_dir Transport direction.
    \param eval_delta If true, evaluate delta function.
    \return Evaluated contribution.

    \rst
    The function evaluates directional component of path integral.
    This function generalizes several functions according to the type of scene interaction.
    For detail, please refer to :ref:`path_evaluating_directional_components`.

    Note that the scene interaction obtained from :cpp:func:`Scene::intersect` or 
    :cpp:func:`Scene::sample_distance` is not an endpont
    even if it might represent either a light or a sensor.
    In this case, you want to use :cpp:func:`lm::SceneInteraction::as_type`
    to enforce an evaluation as an endpoint.
    \endrst
*/
static Vec3 eval_contrb_direction(const Scene* scene, const SceneInteraction& sp, Vec3 wi, Vec3 wo, int comp, TransDir trans_dir, bool eval_delta) {
    const auto& primitive = scene->node_at(sp.primitive).primitive;
    switch (sp.type) {
        case SceneInteraction::CameraEndpoint:
            return primitive.camera->eval(wo);
        case SceneInteraction::LightEndpoint:
            return primitive.light->eval(sp.geom, wo, eval_delta);
        case SceneInteraction::MediumInteraction:
            return primitive.medium->phase()->eval(sp.geom, wi, wo);
        case SceneInteraction::SurfaceInteraction:
            return primitive.material->eval(sp.geom, wi, wo, comp, (Material::TransDir)(trans_dir), eval_delta) *
                   surface::shading_normal_correction(sp.geom, wi, wo, trans_dir);
    }
    LM_UNREACHABLE_RETURN();
}

/*!
    \brief Evaluate reflectance (if available).
    \param scene Scene.
    \param sp Surface interaction.

    \rst
    This function evaluate reflectance if ``sp`` is on a surface
    and the associated material implements :cpp:func:`Material::reflectance` function.
    \endrst
*/
static std::optional<Vec3> reflectance(const Scene* scene, const SceneInteraction& sp) {
    if (!sp.is_type(SceneInteraction::SurfaceInteraction)) {
        LM_THROW_EXCEPTION(Error::Unsupported,
            "reflectance() function only supports surface interactions.");
    }
    const auto& primitive = scene->node_at(sp.primitive).primitive;
    return primitive.material->reflectance(sp.geom);
}

#pragma endregion

/*!
    @}
*/

LM_NAMESPACE_END(path)
LM_NAMESPACE_END(LM_NAMESPACE)
