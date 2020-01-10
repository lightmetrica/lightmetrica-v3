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

    virtual bool is_specular(const PointGeometry& geom, int comp) const override {
        return ref_->is_specular(geom, comp);
    }

    virtual std::optional<MaterialDirectionSample> sample_direction(Rng& rng, const PointGeometry& geom, Vec3 wi) const override {
        return ref_->sample_direction(rng, geom, wi);
    }

    virtual std::optional<Vec3> sample_direction_given_comp(Rng& rng, const PointGeometry& geom, int comp, Vec3 wi) const override {
        return ref_->sample_direction_given_comp(rng, geom, comp, wi);
    }

    virtual std::optional<Vec3> reflectance(const PointGeometry& geom, int comp) const override {
        return ref_->reflectance(geom, comp);
    }

    Float pdf_direction(const PointGeometry& geom, int comp, Vec3 wi, Vec3 wo) const override {
        return ref_->pdf_direction(geom, comp, wi, wo);
    }

    virtual Vec3 eval(const PointGeometry& geom, int comp, Vec3 wi, Vec3 wo) const override {
        return ref_->eval(geom, comp, wi, wo);
    }
};

LM_COMP_REG_IMPL(Material_Proxy, "material::proxy");

LM_NAMESPACE_END(LM_NAMESPACE)
