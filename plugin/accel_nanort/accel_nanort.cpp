/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/accel.h>
#include <lm/scene.h>
#include <lm/exception.h>
#include <nanort.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*
\rst
.. function:: accel::nanort

   Acceleration structure with nanort library.
\endrst
*/
class Accel_NanoRT final : public Accel {
private:
    std::vector<Float> vs_;
    std::vector<unsigned int> fs_;
    nanort::BVHAccel<Float> accel_;
    std::vector<std::tuple<int, int>> primitiveFaceId_;

public:
    virtual void build(const Scene& scene) override {
        // Make a combined mesh
        scene.foreachTriangle([&](int primitive, int face, Vec3 p1, Vec3 p2, Vec3 p3) {
            vs_.insert(vs_.end(), { p1.x, p1.y, p1.z, p2.x, p2.y, p2.z, p3.x, p3.y, p3.z });
            auto s = (unsigned int)(fs_.size());
            fs_.insert(fs_.end(), { s, s+1, s+2 });
            primitiveFaceId_.push_back({ primitive, face });
        });

        // Build acceleration structure
        nanort::BVHBuildOptions<Float> options;
        nanort::TriangleMesh<Float> mesh(vs_.data(), fs_.data(), sizeof(Float) * 3);
        nanort::TriangleSAHPred<Float> pred(vs_.data(), fs_.data(), sizeof(Float) * 3);
        accel_.Build((unsigned int)(fs_.size() / 3), mesh, pred, options);
    }
    
    virtual std::optional<Hit> intersect(Ray ray, Float tmin, Float tmax) const override {
        exception::ScopedDisableFPEx guard_;

        nanort::Ray<Float> r;
        r.org[0] = ray.o[0];
        r.org[1] = ray.o[1];
        r.org[2] = ray.o[2];
        r.dir[0] = ray.d[0];
        r.dir[1] = ray.d[1];
        r.dir[2] = ray.d[2];
        r.min_t = tmin;
        r.max_t = tmax;
        
        nanort::TriangleIntersector<Float> intersector(vs_.data(), fs_.data(), sizeof(Float) * 3);
        nanort::TriangleIntersection<Float> isect;
        if (!accel_.Traverse(r, intersector, &isect)) {
            return {};
        }

        const auto [primitive, face] = primitiveFaceId_[isect.prim_id];
        return Hit{ isect.t, Vec2(isect.u, isect.v), primitive, face };
    }
};

LM_COMP_REG_IMPL(Accel_NanoRT, "accel::nanort");

LM_NAMESPACE_END(LM_NAMESPACE)
