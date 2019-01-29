/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/material.h>
#include <lm/serial.h>
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

public:
    virtual bool construct(const Json& prop) override {
        Ni_ = prop["Ni"];
        return true;
    }

    virtual bool isSpecular(const PointGeometry&) const override {
        return true;
    }

    virtual std::optional<MaterialDirectionSample> sample(Rng& rng, const PointGeometry& geom, Vec3 wi) const override {
        const bool in = glm::dot(wi, geom.n) > 0_f;
        const auto n = in ? geom.n : -geom.n;
        const auto eta = in ? 1_f / Ni_ : Ni_;
        const auto [wt, total] = math::refraction(wi, n, eta);
        const auto Fr = total ? 1_f : [&]() {
            // Flesnel term
            const auto cos = in ? glm::dot(wi, geom.n) : glm::dot(wt, geom.n);
            const auto r = (1_f-Ni_) / (1_f+Ni_);
            const auto r2 = r*r;
            return r2 + (1_f - r2) * std::pow(1_f-cos, 5_f);
        }();
        if (rng.u() < Fr) {
            // Reflection
            return MaterialDirectionSample(
                math::reflection(wi, geom.n),
                0,
                Vec3(1_f)
            );
        }
        // Refraction
        return MaterialDirectionSample(
            wt,
            1,
            Vec3(eta*eta)
        );
    }
};

LM_COMP_REG_IMPL(Material_Glass, "material::glass");

LM_NAMESPACE_END(LM_NAMESPACE)
