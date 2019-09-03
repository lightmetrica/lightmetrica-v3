/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/material.h>
#include <lm/user.h>
#include <lm/serial.h>

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

    virtual void foreachUnderlying(const ComponentVisitor& visit) override {
        comp::visit(visit, ref_);
    }

public:
    virtual bool construct(const Json& prop) override {
        ref_ = comp::get<Material>(prop["ref"]);
        return ref_;
    }

    virtual bool isSpecular(const PointGeometry& geom, int comp) const override {
        return ref_->isSpecular(geom, comp);
    }

    virtual std::optional<MaterialDirectionSample> sample(Rng& rng, const PointGeometry& geom, Vec3 wi) const override {
        return ref_->sample(rng, geom, wi);
    }

    virtual std::optional<Vec3> reflectance(const PointGeometry& geom, int comp) const override {
        return ref_->reflectance(geom, comp);
    }

    Float pdf(const PointGeometry& geom, int comp, Vec3 wi, Vec3 wo) const override {
        return ref_->pdf(geom, comp, wi, wo);
    }

    virtual Float pdfComp(const PointGeometry& geom, int comp, Vec3 wi) const override {
        return ref_->pdfComp(geom, comp, wi);
    }

    virtual Vec3 eval(const PointGeometry& geom, int comp, Vec3 wi, Vec3 wo) const override {
        return ref_->eval(geom, comp, wi, wo);
    }
};

LM_COMP_REG_IMPL(Material_Proxy, "material::proxy");

LM_NAMESPACE_END(LM_NAMESPACE)
