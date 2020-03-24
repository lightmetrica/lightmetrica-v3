/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/material.h>
#include <lm/surface.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*
\rst
.. function:: material::glass

   Fresnel reflection and refraction.

   :param float Ni: Relative index of refraction.

   This component implement Fresnel reflection and refraction BSDF, which reads

   .. math::
      f_s(\omega_i, \omega_o)
        = F\, \delta_\Omega(\omega_{\mathrm{refl}}, \omega_o)
        + (1-F)\, \delta_\Omega(\omega_{\mathrm{refr}}, \omega_o),

   where :math:`F` is Fresnel term and :math:`\delta_\Omega` is
   the Dirac delta function w.r.t. solid angle measure.
   :math:`\omega_{\mathrm{refl}}` and :math:`\omega_{\mathrm{refr}}`
   are reflected and refracted directions of :math:`\omega_i` respectively defined as

   .. math::
      \begin{eqnarray}
        \omega_{\mathrm{refl}}
          &=& 2(\omega_i\cdot\mathbf{n})\mathbf{n} - \omega_i, \\
        \omega_{\mathrm{refr}}
          &=& -\eta\omega_i
                + \left[
                    \eta(\omega_i\cdot\mathbf{n})-\sqrt{1-\eta^2(1-(\omega_i\cdot\mathbf{n})^2)}
                  \right] \mathbf{n},
      \end{eqnarray}

   where :math:`\mathbf{n}` is the shading normal on a position of the scene surface
   and :math:`\eta` is relative index of refraction: :math:`\eta\equiv\frac{n_i}{n_t}`
   where :math:`n_i` and :math:`n_t` is the index of refraction of the media on
   incident and transmitted sides of scene surface respectively.

   For Fresnel term, we used Schlick's approximation [Schlick1994]_:

   .. math::
      \begin{eqnarray}
        F = R_0 + (1-R_0)(1-(\omega_i\cdot\mathbf{n})^2),\;
        R_0 = \left( \frac{1-\eta}{1+\eta} \right)^2.
      \end{eqnarray}

   Reflection or refraction is determined by sampling Fresnel term.

   .. [Schlick1994] C. Schlick.
                    An Inexpensive BRDF Model for Physically-based Rendering.
                    Computer Graphics Forum. 13 (3): 233. 1994.
\endrst
*/
class Material_Glass final : public Material {
private:
    Float Ni_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(Ni_);
    }

private:
    // Energy compensation for importance transport
    Float refr_correction(Float eta, TransDir trans_dir) const {
        return trans_dir == TransDir::EL ? eta * eta : 1_f;
    }

    // Fresnel term
    Float fresnel(Vec3 wi, Vec3 wt, const PointGeometry& geom) const {
        const bool in = glm::dot(wi, geom.n) > 0_f;
        const auto n = in ? geom.n : -geom.n;
        const auto eta = in ? 1_f / Ni_ : Ni_;
        const auto cos = in ? glm::dot(wi, geom.n) : glm::dot(wt, geom.n);
        const auto r = (1_f - Ni_) / (1_f + Ni_);
        const auto r2 = r * r;
        return r2 + (1_f - r2) * std::pow(1_f - cos, 5_f);
    }

    // Weight by Fresnel term
    Float fresnel_weight(const PointGeometry& geom, Vec3 wi, Vec3 wo) const {
        const bool in = glm::dot(wi, geom.n) > 0_f;
        const auto n = in ? geom.n : -geom.n;
        const auto eta = in ? 1_f / Ni_ : Ni_;
        const auto [wt, total] = math::refraction(wi, n, eta);
        const auto Fr = total ? 1_f : fresnel(wi, wt, geom);

        // Choose delta component according to the relashionship of wi and wo
        if (!geom.opposite(wi, wo)) {
            // Reflection
            return Fr;
        }
        else {
            // Refraction
            return 1_f - Fr;
        }
    }

public:
    virtual void construct(const Json& prop) override {
        Ni_ = json::value<Float>(prop, "Ni");
    }

    virtual ComponentSample sample_component(const ComponentSampleU&, const PointGeometry&) const override {
        return { 0, 1_f };
    }

    virtual Float pdf_component(int, const PointGeometry&) const override {
        return 1_f;
    }

    virtual std::optional<DirectionSample> sample_direction(const DirectionSampleU& u, const PointGeometry& geom, Vec3 wi, int, TransDir trans_dir) const override {
        const bool in = glm::dot(wi, geom.n) > 0_f;
        const auto n = in ? geom.n : -geom.n;
        const auto eta = in ? 1_f / Ni_ : Ni_;
        const auto [wt, total] = math::refraction(wi, n, eta);
        const auto Fr = total ? 1_f : fresnel(wi, wt, geom);
        if (u.udc[0] < Fr) {
            // Reflection
            // Fr / p_sel = 1
            const auto wo = math::reflection(wi, geom.n);
            const auto C = Vec3(1_f);
            return DirectionSample{
                wo,
                C
            };
        }
        else {
            // Refraction
            // refr_correction*(1-Fr) / p_sel = refr_correction
            const auto C = Vec3(refr_correction(eta, trans_dir));
            return DirectionSample{
                wt,
                C
            };
        }
    }

    virtual Float pdf_direction(const PointGeometry& geom, Vec3 wi, Vec3 wo, int, bool eval_delta) const override {
        if (eval_delta) {
            return 0_f;
        }

        const bool in = glm::dot(wi, geom.n) > 0_f;
        const auto n = in ? geom.n : -geom.n;
        const auto eta = in ? 1_f / Ni_ : Ni_;
        const auto [wt, total] = math::refraction(wi, n, eta);
        const auto Fr = total ? 1_f : fresnel(wi, wt, geom);

        // Choose delta component according to the relashionship of wi and wo
        if (!geom.opposite(wi, wo)) {
            // Reflection
            return Fr;
        }
        else {
            // Refraction
            return 1_f - Fr;
        }
    }

    virtual Vec3 eval(const PointGeometry& geom, Vec3 wi, Vec3 wo, int, TransDir trans_dir, bool eval_delta) const override {
        if (eval_delta) {
            return Vec3(0_f);
        }

        const bool in = glm::dot(wi, geom.n) > 0_f;
        const auto n = in ? geom.n : -geom.n;
        const auto eta = in ? 1_f / Ni_ : Ni_;
        const auto [wt, total] = math::refraction(wi, n, eta);
        const auto Fr = total ? 1_f : fresnel(wi, wt, geom);

        if (!geom.opposite(wi, wo)) {
            return Vec3(Fr);
        }
        else {
            const auto refr_corr = refr_correction(eta, trans_dir);
            return Vec3((1_f - Fr) * refr_corr);
        }
    }

    virtual std::optional<Vec3> reflectance(const PointGeometry&) const override {
        return Vec3(0_f);
    }

    virtual bool is_specular_component(int) const override {
        return true;
    }
};

LM_COMP_REG_IMPL(Material_Glass, "material::glass");

LM_NAMESPACE_END(LM_NAMESPACE)
