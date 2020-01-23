/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/material.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*
\rst
.. function:: material::proxy

   Proxy material.

   :param str ref: Asset name or locator of the referencing material.
   
   This component gives proxy interface to the other predefined material. 
   This component is useful when we want to reuse predefined material
   but we also need to create a new instance.
\endrst
*/
class Material_Proxy final : public Material {
private:
    Material* ref_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(ref_);
    }

    virtual void foreach_underlying(const ComponentVisitor& visit) override {
        comp::visit(visit, ref_);
    }

public:
    virtual void construct(const Json& prop) override {
        ref_ = json::comp_ref<Material>(prop, "ref");
    }

    virtual std::optional<DirectionSample> sample_direction(const DirectionSampleU& u, const PointGeometry& geom, Vec3 wi, TransDir trans_dir) const override {
        return ref_->sample_direction(u, geom, wi, trans_dir);
    }

    Float pdf_direction(const PointGeometry& geom, Vec3 wi, Vec3 wo, bool eval_delta) const override {
        return ref_->pdf_direction(geom, wi, wo, eval_delta);
    }

    virtual Vec3 eval(const PointGeometry& geom, Vec3 wi, Vec3 wo, TransDir trans_dir, bool eval_delta) const override {
        return ref_->eval(geom, wi, wo, trans_dir, eval_delta);
    }

    virtual std::optional<Vec3> reflectance(const PointGeometry& geom) const override {
        return ref_->reflectance(geom);
    }
};

LM_COMP_REG_IMPL(Material_Proxy, "material::proxy");

LM_NAMESPACE_END(LM_NAMESPACE)
