/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/light.h>
#include <lm/texture.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*
\rst
.. function:: light::env

    Environment light.

    :param str envmap_path: Path to environment map.
    :param float rot: Rotation angle of the environment map around up vector in degrees.
                      Default value: 0.
\endrst
*/
class Light_Env final : public Light {
private:
    Component::Ptr<Texture> envmap_;    // Environment map
    Float rot_;                         // Rotation of the environment map around (0,1,0)
    Dist2 dist_;                        // For sampling directions

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(envmap_, rot_, dist_);
    }

    virtual void foreachUnderlying(const ComponentVisitor& visitor) override {
        comp::visit(visitor, envmap_);
    }

    virtual Component* underlying(const std::string& name) const override {
        if (name == "envmap") {
            return envmap_.get();
        }
        return nullptr;
    }

public:
    virtual bool construct(const Json& prop) override {
        envmap_ = comp::create<Texture>("texture::bitmap", "", prop);
        if (!envmap_) {
            return false;
        }
        rot_ = glm::radians(json::value(prop, "rot", 0_f));
        const auto [w, h] = envmap_->size();
        std::vector<Float> ls(w * h);
        for (int i = 0; i < w*h; i++) {
            const int x = i % w;
            const int y = i / w;
            const auto v = envmap_->evalByPixelCoords(x, y);
            ls[i] = glm::compMax(v) * std::sin(Pi * (Float(i) / w + .5_f) / h);
        }
        dist_.init(ls, w, h);
        return true;
    }

    virtual std::optional<LightRaySample> sample(Rng& rng, const PointGeometry& geom, const Transform&) const override {
        const auto u = dist_.samp(rng);
        const auto t  = Pi * u[1];
        const auto st = sin(t);
        const auto p  = 2 * Pi * u[0] + rot_;
        const auto wo = -Vec3(st * sin(p), cos(t), st * cos(p));
        const auto geomL = PointGeometry::makeInfinite(wo);
        const auto pL = pdf(geom, geomL, 0, {}, wo);
        if (pL == 0_f) {
            return {};
        }
        const auto Le = eval(geomL, 0, wo);
        return LightRaySample{
            geomL,
            wo,
            0,
            Le / pL
        };
    }

    virtual Float pdf(const PointGeometry& geom, const PointGeometry& geomL, int, const Transform&, Vec3) const override {
        const auto d  = -geomL.wo;
        const auto at = [&]() {
            const auto at = std::atan2(d.x, d.z);
            return at < 0_f ? at + 2_f * Pi : at;
        }();
        const auto t = (at - rot_) * .5_f / Pi;
        const auto u = t - std::floor(t);
        const auto v = std::acos(d.y) / Pi;
        const auto st = math::safeSqrt(1 - d.y * d.y);
        if (st == 0_f) {
            return 0_f;
        }
        const auto pdfSA = dist_.p(u, v) / (2_f*Pi*Pi*st);
        return surface::convertSAToProjSA(pdfSA, geom, d);
    }

    virtual bool isSpecular(const PointGeometry&, int) const override {
        return false;
    }

    virtual bool isInfinite() const override {
        return true;
    }

    virtual Vec3 eval(const PointGeometry& geom, int, Vec3) const override {
        const auto d = -geom.wo;
        const auto at = [&]() {
            const auto at = std::atan2(d.x, d.z);
            return at < 0_f ? at + 2_f * Pi : at;
        }();
        const auto t = (at - rot_) * .5_f / Pi;
        return envmap_->eval({ t - floor(t), acos(d.y) / Pi });
    }
};

LM_COMP_REG_IMPL(Light_Env, "light::env");

LM_NAMESPACE_END(LM_NAMESPACE)
