/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/


#include <lm/lm.h>

#define LM_LIGHT_ENV_PORTAL_AVOID_FIREFRIES 1

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// 2D bound
struct Bound2 {
    Vec2 mi = Vec2(Inf);
    Vec2 ma = Vec2(-Inf);

    Float area() const {
        return (ma.x-mi.x)*(ma.y-mi.y);
    }

    bool contains(Vec2 p) const {
        return mi.x <= p.x && p.x <= ma.x && mi.y <= p.y && p.y <= ma.y;
    }

    friend Bound2 merge(Bound2 b, Vec2 p) {
        return { glm::min(b.mi, p), glm::max(b.ma, p) };
    }

    friend Bound2 merge(Bound2 a, Bound2 b) {
        return { glm::min(a.mi, b.mi), glm::max(a.ma, b.ma) };
    }
};

// ----------------------------------------------------------------------------

// Face index of a portal
namespace FaceIndex {
    enum {
        Front = 0,  // Seen from the front
        Back = 1,   // Seen from the back
    };
};

// Portal
struct Portal {
    std::vector<Vec3> ps;
    Vec3 ex, ey, ez;
    Mat3 toWorld;
    Mat3 toLocal;

    Portal() = default;
    Portal(const std::vector<Vec3>& ps) : ps(ps) {
        // ex and ey must be orthogonal (we don't check it)
        ex = glm::normalize(ps[1] - ps[0]);
        ey = glm::normalize(ps[3] - ps[0]);
        ez = glm::normalize(glm::cross(ex, ey));
        toWorld = Mat3(ex, ey, ez);
        toLocal = glm::transpose(toWorld);
    }

    // Check how the portal is seen from the shading point
    // Front: Portal is facing toward the shading point
    // Back : Portal is facing opposite to the shading point
    int checkFrontOrBackFace(Vec3 shadingPoint) const {
        return glm::dot(ez, shadingPoint - ps[0]) > 0 ? FaceIndex::Front : FaceIndex::Back;
    }

    // Convert a direction in world coordinates to canonical coordinates
    Vec3 rectifiedToCanonical(Vec2 p_rect, int face) const {
        const lm::Vec2 p_cano(std::tan(p_rect.x), std::tan(p_rect.y));
        const auto d_cano = glm::normalize(lm::Vec3(p_cano, face == FaceIndex::Front ? -1_f : 1_f));
        return d_cano;
    }

    // Convert rectified coordinates to the direction in world coordinates
    Vec3 rectifiedtoWorldDir(Vec2 p_rect, int face) const {
        const auto d_cano = rectifiedToCanonical(p_rect, face);
        const auto d_world = toWorld * d_cano;
        return d_world;
    }

    // Convert a direction in world coordinates to rectified coordinates 
    // d_world doesn't need to be normalized
    Vec2 worldDirToRectified(Vec3 d_world, int face) const {
        // To local
        const auto p_local = toLocal * d_world;

        // To canonical coordinates
        Vec3 p_cano = face == FaceIndex::Front
            ? -p_local / p_local.z    // Front: canonical plane z=-1
            : p_local / p_local.z;    // Back : canonical plane z=1

        // To rectified coordinates
        return Vec2(atan(p_cano.x), atan(p_cano.y));
    }

    // Compute the extent of the portal in rectified coordinates
    Bound2 rectifiedPortalBound(Vec3 shadingPoint, int face) const {
        Bound2 b_rect;
        for (int i = 0; i < 4; i++) {
            const auto p = worldDirToRectified(ps[i] - shadingPoint, face);
            b_rect = merge(b_rect, p);
        }
        return b_rect;
    }
};

// ----------------------------------------------------------------------------

// 2D continuous piecewise-constant distribution for the subregion
struct Dist2Sub {
    std::vector<Float> sat;
    int w, h;

    void init(const std::vector<Float>& v, int cols, int rows) {
        // Compute SAT
        w = cols;
        h = rows;
        sat.assign((w+1)*(h+1), 0_f);
        for (int y = 1; y <= h; y++) {
            for (int x = 1; x <= w; x++) {
                sat[si(x,y)] = v[(y-1)*w+x-1] + sat[si(x-1,y)] + sat[si(x,y-1)] - sat[si(x-1,y-1)];
            }
        }
    }

    Float R(Vec2 mi, Vec2 ma) const {
        return S(ma.x, ma.y) - S(mi.x, ma.y) - S(ma.x, mi.y) + S(mi.x, mi.y);
    }

    // x \in [0,w], y \in [0,h]
    Float cdfX(Float x, const Bound2& b) const {
        const auto R1 = R(b.mi, Vec2(x, b.ma.y));
        const auto R2 = R(b.mi, b.ma);
        return R1 / R2;
    }

    Float cdfYGivenX(Float y, Float x, const Bound2& b) const {
        const int xl = glm::clamp(int(x), 0, w-1);
        const int xu = xl+1;
        const auto R1 = R(Vec2(xl, b.mi.y), Vec2(xu, y));
        const auto R2 = R(Vec2(xl, b.mi.y), Vec2(xu, b.ma.y));
        return R1 / R2;
    }

    Float pdf(Float u1, Float u2, const Bound2& b) const {
        const auto x = b.mi.x+(b.ma.x-b.mi.x)*u1;
        const auto y = b.mi.y+(b.ma.y-b.mi.y)*u2;
        const int xl = glm::clamp(int(x), 0, w-1);
        const int yl = glm::clamp(int(y), 0, h-1);
        const int xu = xl+1;
        const int yu = yl+1;
        const auto pX = cdfX(xu,b)-cdfX(xl,b);
        const auto pYGivenX = cdfYGivenX(yu,x,b)-cdfYGivenX(yl,x,b);
        return pX * pYGivenX * b.area();
    }

    Vec2 sample(Rng& rng, const Bound2& b) const {
        const auto x = sample1D(rng, b, b.mi.x, b.ma.x, [&](Float v, const Bound2& b) {
            return cdfX(v, b);
        });
        const auto y = sample1D(rng, b, b.mi.y, b.ma.y, [&](Float v, const Bound2& b) {
            return cdfYGivenX(v, x, b);
        });
        return Vec2((x - b.mi.x) / (b.ma.x-b.mi.x), (y - b.mi.y) / (b.ma.y-b.mi.y));
    }

public:
    int si(int x, int y) const {
        return y * (w + 1) + x;
    }
    
    Float S(Float x, Float y) const {
        // p \in [xl,yl] \times [xu,yu]
        const int xl = glm::clamp(int(x), 0, w-1);
        const int yl = glm::clamp(int(y), 0, h-1);
        const int xu = xl+1;
        const int yu = yl+1;
        const auto s00 = sat[si(xl,yl)];
        const auto s01 = sat[si(xl,yu)];
        const auto s10 = sat[si(xu,yl)];
        const auto s11 = sat[si(xu,yu)];
        const auto t1 = s00;
        const auto t2 = (y-yl)*(s01-s00);
        const auto t3 = (x-xl)*(s10-s00);
        const auto t4 = (x-xl)*(y-yl)*(s11-s10-s01+s00);
        return t1+t2+t3+t4;
    }

    using CDFFunc = std::function<Float(Float v, const Bound2&)>;
    Float sample1D(Rng& rng, const Bound2& b, Float lb_, Float ub_, const CDFFunc& cdf) const {
        int lb = int(lb_);
        int ub = int(ub_)+1;
        const auto u = rng.u();
        while (ub - lb > 1) {
            const auto m = (ub + lb) / 2;
            if (cdf(m, b) > u) {
                ub = m;
            }
            else {
                lb = m;
            }
        }
        lb_ = glm::max(lb_, Float(lb));
        ub_ = glm::min(ub_, Float(ub));
        const auto cdf_lb = cdf(lb_, b);
        const auto cdf_ub = cdf(ub_, b);
        return lb_ + (ub_ - lb_) * (u - cdf_lb) / (cdf_ub - cdf_lb);
    }
};

// ----------------------------------------------------------------------------

// Width and height of the precomputed distributions
constexpr int DistSize = 4096;

// Information associated with a single portal
struct PortalContext {
    Portal portal;                      // Portal
    Dist2Sub dist[2];                   // SATs for front and back faces
    std::vector<Vec3> rectEnvmap[2];    // Rectified envmap for front and back faces
};

// ----------------------------------------------------------------------------

class Light_EnvPortal final : public Light {
private:
    Component::Ptr<Texture> envmap_;
    Float rot_;
    std::vector<PortalContext> portals_;

public:
    LM_SERIALIZE_IMPL(ar) {
        LM_UNUSED(ar);
        throw std::runtime_error("Not supported");
    }

    virtual void foreachUnderlying(const ComponentVisitor& visitor) override {
        comp::visit(visitor, envmap_);
    }

private:
    // Convert rectified coodinates to the coodinates used in precomputed distribution
    Bound2 rectifiedToDist(Bound2 p_rect) const {
        const auto piover2 = Pi * .5_f;
        return {
            (p_rect.mi/piover2+1_f)*.5_f*Float(DistSize),
            (p_rect.ma/piover2+1_f)*.5_f*Float(DistSize)
        };
    }

    // Create a distribution to select a portal
    Dist selectionDist(const PointGeometry& geom) const {
        Dist selectionDist;
        for (int i = 0; i < (int)(portals_.size()); i++) {
            const auto& portal = portals_[i];
            const int face = portal.portal.checkFrontOrBackFace(geom.p);
            const auto& dist = portal.dist[face];
            const auto b_rect = portal.portal.rectifiedPortalBound(geom.p, face);
            const auto I = dist.R(b_rect.mi, b_rect.ma);
            selectionDist.add(I);
        }
        selectionDist.norm();
        return selectionDist;
    }

public:
    virtual bool construct(const Json& prop) override {
        // Load environment map
        envmap_ = comp::create<Texture>("texture::bitmap", "", prop);
        if (!envmap_) {
            return false;
        }
        rot_ = glm::radians(json::value(prop, "rot", 0_f));

        // Portal
        auto ps = prop["portal"];
        for (const auto& p : ps) {
            portals_.emplace_back();
            portals_.back().portal = Portal({ p[0], p[1], p[2], p[3] });
        }
        
        // Precompute 2D distributions of the intensities of environment light
        // in rectified coordinates seen from the front and the back respectively.
        for (auto& portal : portals_) {
            for (int face = 0; face <= 1; face++) {
                auto& rectEnvmap = portal.rectEnvmap[face];
                rectEnvmap.assign(DistSize*DistSize, Vec3());

                // Compute environment map in rectified coordinates
                std::vector<Float> ls(DistSize*DistSize);
                for (int y = 0; y < DistSize; y++) {
                    for (int x = 0; x < DistSize; x++) {
                        // Rectified coodinates ranges in [-pi/2,pi/2]
                        Vec2 p_rect(
                            (2_f*(x + .5_f) / DistSize - 1_f)*Pi*.5_f,
                            (2_f*(y + .5_f) / DistSize - 1_f)*Pi*.5_f);

                        // To world
                        const auto d_world = portal.portal.rectifiedtoWorldDir(p_rect, face);

                        // Query environment map
                        const auto C = eval(PointGeometry::makeInfinite(-d_world), 0, -d_world);

                        // Record contribution
                        rectEnvmap[y*DistSize + x] = C;
                        ls[y*DistSize + x] = glm::compMax(C);
                    }
                }

                // Create 2D distribution
                portal.dist[face].init(ls, DistSize, DistSize);
            }
        }
        
        return true;
    }

    virtual std::optional<LightRaySample> sample(Rng& rng, const PointGeometry& geom, const Transform&) const override {
        // Create a distribution to select a portal
        const auto sdist = selectionDist(geom);
        
        // Randomly select a portal
        const int portalIndex = sdist.samp(rng);
        const auto& portal = portals_[portalIndex];

        // Check how the portal is seen from the shading point
        const int face = portal.portal.checkFrontOrBackFace(geom.p);
        const auto& dist = portal.dist[face];

        // Compute the extent of the portal in rectified coordinates
        const auto b_rect = portal.portal.rectifiedPortalBound(geom.p, face);

        // Sample a position on the portal in rectified coordinates
        const auto b_dist = rectifiedToDist(b_rect);
        const auto p_rect = b_rect.mi + dist.sample(rng, b_dist) * (b_rect.ma - b_rect.mi);

        // Convert the point back to the world coordinates
        const auto d_world = portal.portal.rectifiedtoWorldDir(p_rect, face);

        // Direction from the environment light and point geometry
        const auto wo = -d_world;
        const auto geomL = PointGeometry::makeInfinite(wo);

        // Evaluate pdf
        const auto pL = pdf(geom, geomL, portalIndex, {}, wo);
        if (pL == 0_f) {
            return {};
        }

        // Evaluate contribution
        #if !LM_LIGHT_ENV_PORTAL_AVOID_FIREFRIES
        const auto Le = eval(geomL, wo);
        #else
        // We use rectified envmap to evaluate luminance instead of using original envmap.
        // A pixel footprint of the rectified envmap might be mapped to
        // the region in the environment map that overlaps the edge between bright and non-bright pixels. 
        // This means large portion of the samples are taken for that pixel in rectified envmap
        // can be mapped to the less-bright part of the pixel in the original envmap,
        // which can be a cause of firefries.
        const auto Le = [&] {
            // Pixel position of rectified envmap
            const auto piover2 = Pi * .5_f;
            const auto t = (p_rect/piover2+1_f)*.5_f*Float(DistSize);
            const int x = glm::clamp(int(t.x), 0, dist.w-1);
            const int y = glm::clamp(int(t.y), 0, dist.h-1);
            return portal.rectEnvmap[face][y*DistSize+x];
        }();
        #endif
        return LightRaySample{
            geomL,
            wo,
            portalIndex,
            Le / pL
        };
    }

    virtual Float pdf(const PointGeometry& geom, const PointGeometry& geomL, int comp, const Transform&, Vec3) const override {
        // Component index represents portal index
        const int portalIndex = comp;

        // PDF of sampling the direction with the selected portal
        const auto p_portal = [&]() -> Float {
            const auto& portal = portals_[portalIndex];

            // Portal face orientation
            const int face = portal.portal.checkFrontOrBackFace(geom.p);

            // Direction in world coordinates to rectified coordinates
            const auto d_world = -geomL.wo;
            const auto p_rect = portal.portal.worldDirToRectified(d_world, face);
        
            // Compute the extent of the portal in rectified coordinates
            const auto b_rect = portal.portal.rectifiedPortalBound(geom.p, face);

            // Check if p_rect is inside the portal
            if (!b_rect.contains(p_rect)) {
                return 0_f;
            }

            // Compute pdf
            const auto piover2 = Pi * .5_f;
            const auto p_dist = (p_rect-b_rect.mi)/(b_rect.ma-b_rect.mi);  // Be careful of the range
            const auto b_dist = rectifiedToDist(b_rect);
            const auto p = portal.dist[face].pdf(p_dist.x, p_dist.y, b_dist) / b_rect.area();
        
            // Jacobian
            const auto d_cano = portal.portal.rectifiedToCanonical(p_rect, face);
            const auto J = std::abs(d_cano.z) / ((1_f - d_cano.x*d_cano.x) * (1_f - d_cano.y*d_cano.y));

            return p * J / glm::abs(glm::dot(d_world, geom.n));
        }();
        
        // Selection probability of the portal
        const auto sdist = selectionDist(geom);
        const auto p_sel = sdist.p(portalIndex);

        // Count the number of overlapping portals
        int overlappingPortals = 1;
        for (int i = 0; i < (int)(portals_.size()); i++) {
            if (i == portalIndex) {
                continue;
            }
            const auto& portal = portals_[i];
            const int face = portal.portal.checkFrontOrBackFace(geom.p);
            const auto d_world = -geomL.wo;
            const auto p_rect = portal.portal.worldDirToRectified(d_world, face);
            const auto b_rect = portal.portal.rectifiedPortalBound(geom.p, face);
            if (b_rect.contains(p_rect)) {
                overlappingPortals++;
            }
        }

        // MIS weight
        const auto inv_misw = (Float)(overlappingPortals);

        // Solid angle measure to projected solid anglme measure
        return p_portal * p_sel * inv_misw;
    }

    virtual bool isSpecular(const PointGeometry&) const override {
        return false;
    }

    virtual bool isInfinite() const override {
        return true;
    }

    virtual Vec3 eval(const PointGeometry& geom, Vec3) const override {
        const auto d = -geom.wo;
        const auto at = [&]() {
            const auto at = std::atan2(d.x, d.z);
            return at < 0_f ? at + 2_f * Pi : at;
        }();
        const auto t = (at - rot_) * .5_f / Pi;
        return envmap_->eval({ t - floor(t), acos(d.y) / Pi });
    }
};

LM_COMP_REG_IMPL(Light_EnvPortal, "light::env_portal");

LM_NAMESPACE_END(LM_NAMESPACE)
