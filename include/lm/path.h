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
/*
    Samples the joint distribution of the scene interaction x and the direction wo
    originated from x. (x, wo) ~ p(x, wo).
*/

/*!
    \brief Result of ray sampling.

    \rst
    This structure represents the result of ray sampling
    used by the functions of :cpp:class:`lm::Scene` class.
    \endrst
*/
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
        assert(!sp.geom.infinite);
        return { sp.geom.p, wo };
    }
};

/*!
    \brief Generate a primary ray.
    \param rp Raster position in [0,1]^2.
    \param aspect Aspect ratio of the film.
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

/*!
*/
struct RaySampleU {
    union {
        struct {
            Vec2 up;    // For position
            Vec2 upc;   // For positional component
            Vec2 ud;    // For direction
            Vec2 udc;   // For directional component
        };
        Float data[8];
    };
};

/*!
    \brief Sample a ray given surface point and incident direction.
    \param rng Random number generator.
    \param sp Surface interaction.
    \param wi 

    \rst
    This function samples a ray given the scene interaction.
    According to the types of scene interaction, this function samples a different
    types of the ray from the several distributions.

    (1) If the scene interaction is ``terminator``, this function samples a primary ray
        according type types of the terminator (camera or light). ``wi`` is ignored in this case.

    (2) If the scene interaction is not ``terminator``, this function samples
        a ray from the associated distribution to BSDF or phase function
        given the interaction event ``sp`` and incident ray direction ``wi``.

    In both cases, this function returns ``nullopt`` if the sampling failed,
    or the case when the early return is possible for instance when 
    the evaluated contribution of the sampled direction is zero.
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
            s->weight,
            s->specular
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
            s->weight,
            s->specular
        };
    }
    LM_UNREACHABLE_RETURN();
}

/*!
*/
static std::optional<RaySample> sample_primary_ray(Rng& rng, const Scene* scene, TransDir trans_dir) {
    return sample_primary_ray(rng.next<RaySampleU>(), scene, trans_dir);
}

#pragma endregion

// ------------------------------------------------------------------------------------------------

#pragma region Position sampling

/*!
*/
struct PositionSample {
    SceneInteraction sp;
    Vec3 weight;
};

/*!
*/
struct PositionSampleU {
    Vec2 up;
    Vec2 upc;
};

/*!
*/
static std::optional<PositionSample> sample_position(const PositionSampleU& u, const Scene* scene, TransDir trans_dir) {
    if (trans_dir == TransDir::EL) {
        const auto* camera = scene->node_at(scene->camera_node()).primitive.camera;
        const auto s = camera->sample_position();
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
*/
static std::optional<PositionSample> sample_position(Rng& rng, const Scene* scene, TransDir trans_dir) {
    return sample_position(rng.next<PositionSampleU>(), scene, trans_dir);
}

#pragma endregion

// ------------------------------------------------------------------------------------------------

#pragma region Direction sampling

/*
    Samples a direction wo ~ p_{\sigma^\bot}(wo).
*/

/*!
*/
struct DirectionSample {
    Vec3 wo;
    Vec3 weight;
    bool specular;
};

/*!
*/
struct DirectionSampleU {
    Vec2 ud;
    Vec2 udc;
};

/*!
*/
static std::optional<DirectionSample> sample_direction(const DirectionSampleU& u, const Scene* scene, const SceneInteraction& sp, Vec3 wi, TransDir trans_dir) {
    const auto& primitive = scene->node_at(sp.primitive).primitive;
    if (sp.is_type(SceneInteraction::CameraEndpoint)) {
        const auto s = primitive.camera->sample_direction({ u.ud });
        if (!s) {
            return {};
        }
        return DirectionSample{
            s->wo,
            s->weight,
            s->specular
        };
    }
    else if (sp.is_type(SceneInteraction::LightEndpoint)) {
        const auto s = primitive.light->sample_direction(sp.geom, { u.ud });
        if (!s) {
            return {};
        }
        return DirectionSample{
            s->wo,
            s->weight,
            s->specular
        };
    }
    else if (sp.is_type(SceneInteraction::MediumInteraction)) {
        const auto s = primitive.medium->phase()->sample_direction({u.ud}, sp.geom, wi);
        if (!s) {
            return {};
        }
        return DirectionSample{
            s->wo,
            s->weight,
            s->specular
        };
    }
    else if (sp.is_type(SceneInteraction::SurfaceInteraction)) {
        const auto s = primitive.material->sample_direction({u.ud,u.udc}, sp.geom, wi, static_cast<Material::TransDir>(trans_dir));
        if (!s) {
            return {};
        }
        const auto sn_corr = surface::shading_normal_correction(sp.geom, wi, s->wo, trans_dir);
        return DirectionSample{
            s->wo,
            s->weight * sn_corr,
            s->specular
        };
    }
    LM_UNREACHABLE_RETURN();  
};

/*!
*/
static std::optional<DirectionSample> sample_direction(Rng& rng, const Scene* scene, const SceneInteraction& sp, Vec3 wi, TransDir trans_dir) {
    return sample_direction(rng.next<DirectionSampleU>(), scene, sp, wi, trans_dir);
}

/*!
    \brief Evaluate pdf for direction sampling.
    \param sp Scene interaction.
	\param comp Component index.
    \param wi Incident ray direction.
    \param wo Sampled outgoing ray direction.
    \return Evaluated pdf.

    \rst
    This function evaluates pdf according to *projected solid angme measure* if ``sp.geom.degenerated=false``
    and *solid angme measure* if ``sp.geom.degenerated=true``
    utlizing corresponding densities from which the direction is sampled.
    \endrst
*/
static Float pdf_direction(const Scene* scene, const SceneInteraction& sp, Vec3 wi, Vec3 wo, bool eval_delta) {
    const auto& primitive = scene->node_at(sp.primitive).primitive;
    switch (sp.type) {
        case SceneInteraction::CameraEndpoint:
            return primitive.camera->pdf_direction(wo);
        case SceneInteraction::LightEndpoint:
            return primitive.light->pdf_direction(sp.geom, wo);
        case SceneInteraction::MediumInteraction:
            return primitive.medium->phase()->pdf_direction(sp.geom, wi, wo);
        case SceneInteraction::SurfaceInteraction:
            return primitive.material->pdf_direction(sp.geom, wi, wo, eval_delta);
    }
    LM_UNREACHABLE_RETURN();
}

/*!
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

#pragma region Direct endpoint sampling

/*
    Samples a direction to a light or a sensor given a current x.
    wo ~ p_{\sigma^\bot}(wo | x)
*/

/*!
    \brief Sample direction to a light given a scene interaction.
    \param rng Random number generator.
    \param sp Scene interaction.

    \rst
    This function samples a ray to the light given a scene interaction.
    Be careful not to confuse the sampled ray with the ray sampled via :cpp:func:`Scene::sample_ray`
    function from a light source. Both rays are sampled from the different distributions
    and if you want to evaluate densities you want to use different functions.
    \endrst
*/
static std::optional<RaySample> sample_direct_light(const RaySampleU& u, const Scene* scene, const SceneInteraction& sp) {
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
        s->weight / p_sel,
        s->specular
    };
}

/*!
*/
static std::optional<RaySample> sample_direct_light(Rng& rng, const Scene* scene, const SceneInteraction& sp) {
    return sample_direct_light(rng.next<RaySampleU>(), scene, sp);
}

/*!
*/
static std::optional<RaySample> sample_direct_camera(const RaySampleU& u, const Scene* scene, const SceneInteraction& sp) {
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
        s->weight,
        s->specular
    };
}

/*!
*/
static std::optional<RaySample> sample_direct_camera(Rng& rng, const Scene* scene, const SceneInteraction& sp) {
    return sample_direct_camera(rng.next<RaySampleU>(), scene, sp);
}

/*!
    \brief Evaluate pdf for endpoint sampling given a scene interaction.
    \param sp Scene interaction.
    \param sp_endpoint Sampled scene interaction of the endpoint.
	\param comp_endpoint Component index of the endpoint.
    \param wo Sampled outgoing ray directiom *from* the endpoint.

    \rst
    This function evaluate pdf for the ray sampled via :cpp:func:`Scene::sample_direct_light`
    or :cpp:func:`Scene::sample_direct_camera`.
    Be careful ``wo`` is the outgoing direction originated from ``sp_endpoint``, not ``sp``.
    \endrst
*/
static Float pdf_direct(const Scene* scene, const SceneInteraction& sp, const SceneInteraction& sp_endpoint, Vec3 wo) {
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
            sp.geom, sp_endpoint.geom, light_primitive_index.global_transform, wo);
        return pL_sel * pL_pos;
    }

    LM_UNREACHABLE_RETURN();
}

#pragma endregion

// ------------------------------------------------------------------------------------------------

#pragma region Distance sampling

/*!
    \brief Result of distance sampling.
*/
struct DistanceSample {
    SceneInteraction sp;    //!< Sampled interaction point.
    Vec3 weight;            //!< Contribution divided by probability.
};

/*!
    \brief Sample a distance in a ray direction.
    \param rng Random number generator.
    \param sp Scene interaction.
    \param wo Ray direction.

    \rst
    This function samples either a point in a medium or a point on the surface.
    Note that we don't provide corresponding pdf function because
    some underlying distance sampling technique might not have the analitical representation.
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
        return DistanceSample{
            *hit,
            ds ? ds->weight : Vec3(1_f)
        };
    }
}

/*!
    \brief Evaluate transmittance.
    \param rng Random number generator.
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
*/
static bool is_specular_any(const Scene* scene, const SceneInteraction& sp) {
    const auto& primitive = scene->node_at(sp.primitive).primitive;
    switch (sp.type) {
        case SceneInteraction::SurfaceInteraction:
            return primitive.material->is_specular_any();
    }
    LM_UNREACHABLE_RETURN();
}

/*!
    \brief Compute a raster position.
    \param wo Primary ray direction.
    \param aspect Aspect ratio of the film.
    \return Raster position.
*/
static std::optional<Vec2> raster_position(const Scene* scene, Vec3 wo) {
    const auto* camera = scene->node_at(scene->camera_node()).primitive.camera;
    return camera->raster_position(wo);
}

/*!
    \brief Evaluate directional contribution.
    \param sp Scene interaction.
	\param comp Component index.
    \param wi Incident ray direction.
    \param wo Outgoing ray direction.
    \return Evaluated contribution.

    \rst
    This function evaluate directional contribution according to the scene interaction type.
        
    (1) If the scene interaction is endpoint and on a light,
        this function evaluates luminance function.

    (2) If the scene interaction is endpoint and on a sensor,
        this function evaluates importance function.
        
    (3) If the scene interaction is not endpoint and on a surface,
        this function evaluates BSDF.

    (4) If the scene interaction is in a medium,
        this function evaluate phase function.

    Note that the scene interaction obtained from :cpp:func:`Scene::intersect` or 
    :cpp:func:`Scene::sample_distance` is not an endpont
    even if it might represent either a light or a sensor.
    In this case, you want to use :cpp:func:`Scene::eval_contrb_position`
    to enforce an evaluation as an endpoint.
    \endrst
*/
static Vec3 eval_contrb_direction(const Scene* scene, const SceneInteraction& sp, Vec3 wi, Vec3 wo, TransDir trans_dir, bool eval_delta) {
    const auto& primitive = scene->node_at(sp.primitive).primitive;
    switch (sp.type) {
        case SceneInteraction::CameraEndpoint:
            return primitive.camera->eval(wo);
        case SceneInteraction::LightEndpoint:
            return primitive.light->eval(sp.geom, wo);
        case SceneInteraction::MediumInteraction:
            return primitive.medium->phase()->eval(sp.geom, wi, wo);
        case SceneInteraction::SurfaceInteraction:
            return primitive.material->eval(sp.geom, wi, wo, (Material::TransDir)(trans_dir), eval_delta) *
                   surface::shading_normal_correction(sp.geom, wi, wo, trans_dir);
    }
    LM_UNREACHABLE_RETURN();
}

/*!
    \brief Evaluate positional contribution of the endpoint.
*/
static Vec3 eval_contrb_position(const Scene* scene, const SceneInteraction& sp) {
    if (!sp.is_type(SceneInteraction::Endpoint)) {
        LM_THROW_EXCEPTION(Error::Unsupported,
            "eval_contrb_position() function only supports endpoint interactions.");
    }
    // Always 1 for now
    LM_UNUSED(scene);
    return Vec3(1_f);
}

/*!
    \brief Evaluate reflectance (if available).
    \param sp Surface interaction.
	\param comp Component index.

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
