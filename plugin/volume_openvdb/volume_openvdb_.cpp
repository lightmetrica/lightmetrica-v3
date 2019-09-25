/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/volume.h>
#include <lm/core.h>
#pragma warning(push)
#pragma warning(disable:4211)	// nonstandard extension used: redefined extern to static
#pragma warning(disable:4244)	// possible loss of data
#pragma warning(disable:4275)	// non dll-interface class used as base for dll-interface class
#pragma warning(disable:4251)	// needs to have dll-interface to be used by clients
#pragma warning(disable:4146)	// unary minus operator applied to unsigned type, result still unsigned
#pragma warning(disable:4127)	// conditional expression is constant
#pragma warning(disable:4706)	// assignment within conditional expression
#include <openvdb/openvdb.h>
#include <openvdb/math/Ray.h>
#include <openvdb/math/DDA.h>
#include <openvdb/tools/Interpolation.h>
#include <openvdb/tools/Morphology.h>
#include <openvdb/tools/Prune.h>
#pragma warning(pop)

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

namespace {
// Convert to OpenVDB's corresponding data types
openvdb::math::Vec3<Float> toVDBVec3(Vec3 v) {
    return openvdb::math::Vec3(v.x, v.y, v.z);
}
openvdb::math::Ray<Float> toVDBRay(Ray r) {
    return openvdb::math::Ray<Float>(toVDBVec3(r.o), toVDBVec3(r.d));
}
Vec3 toLMVec3(const openvdb::math::Vec3<Float>& v) {
    return Vec3(v.x(), v.y(), v.z());
}
Bound toLMBound(const openvdb::math::CoordBBox& b) {
    return { toLMVec3(b.min().asVec3d()), toLMVec3(b.max().asVec3d()) };
}
Bound toLMBound(const openvdb::BBoxd& b) {
    return { toLMVec3(b.min()), toLMVec3(b.max()) };
}
}

class Volume_OpenVDBScalar_ : public Volume {
private:
    using GridT = openvdb::FloatGrid;
    GridT::Ptr grid_;
    openvdb::CoordBBox vdbBound_index_;	// Bound in the volume space
    Bound bound_;						// Bound in the world space
    Float maxScalar_;                   // Maximum density
    Float scale_;

public:
    virtual bool construct(const Json& prop) override {
        // Initialize OpenVDB if not initialized
        openvdb::initialize();

        // Path to the volume
        const auto path = json::value<std::string>(prop, "path");
        
        // Load grid (find first one)
        LM_INFO("Opening OpenVDB file [path='{}']", path);
        LM_INDENT();
        openvdb::io::File file(path);
        file.open(false);

        auto grids = file.readAllGridMetadata();
        for (size_t i = 0; i < grids->size(); i++) {
            auto grid = openvdb::gridPtrCast<openvdb::FloatGrid>(grids->at(i));
            if (grid) {
                const auto gridName = grid->getName();
                LM_INFO("Found a grid [name='{}']", grid->getName());
                // Note that readAllGridMetadata() function only reads the metadata
                // so we need to read the grid again by the name being found.
                file.close();
                file.open();
                grid_ = openvdb::gridPtrCast<openvdb::FloatGrid>(file.readGrid(gridName));
                break;
            }
        }
        if (!grid_) {
            LM_ERROR("Floating-point grid is not found");
            return false;
        }

        // Some volume data is z-up
        const bool zup = json::value<bool>(prop, "zup", false);
        if (zup) {
            grid_->transform().postRotate(glm::radians(-90.0), openvdb::math::X_AXIS);
        }

        // Compute AABB of the grid
        // evalActiveVoxelBoundingBox() function computes the bound in the index space
        // so we need to transform it to the world space.
        vdbBound_index_ = grid_->evalActiveVoxelBoundingBox();
        const auto vdbBound_world = grid_->constTransform().indexToWorld(vdbBound_index_);
        bound_ = toLMBound(vdbBound_world);

        // Density scale
        scale_ = json::value<Float>(prop, "scale", 1_f);

        // Minumum and maximum values
        // Be careful not to call evalMinMax() every invocation of
        // maxScalar() function because the function traverses entire tree.
        float min, max;
        grid_->evalMinMax(min, max);
        LM_INFO("Minimum value = {}", min);
        LM_INFO("Maximum value = {}", max);
        maxScalar_ = max * scale_;

        return true;
    }

    virtual Bound bound() const override {
        return bound_;
    }

    virtual Float maxScalar() const override {
        return maxScalar_;
    }

    virtual bool hasScalar() const override {
        return true;
    }

    virtual Float evalScalar(Vec3 p) const override {
        using SamplerT = openvdb::tools::GridSampler<GridT, openvdb::tools::BoxSampler>;
        SamplerT sampler(*grid_);
        const auto d = sampler.wsSample(toVDBVec3(p));
        return d * scale_;
    }

    virtual bool hasColor() const override {
        return false;
    }

    virtual void march(Ray ray, Float tmin, Float tmax, Float marchStep, const RayMarchFunc& rayMarchFunc) const override {
        exception::ScopedDisableFPEx guard_;
        
        // Ray in the world space
        auto vdbRay_world = toVDBRay(ray);
        vdbRay_world.setTimes(tmin, tmax);

        // Ray in the index space (volume space)
        auto vdbRay_index = vdbRay_world.worldToIndex(*grid_);
        
        // Check intersection with the bound
        const bool hit = vdbRay_index.clip(vdbBound_index_);
        if (!hit) {
            return;
        }
        auto tmax_index = vdbRay_index.t1();

        // Scale to convert length between spaces
        const auto length_indexToWorld = grid_->indexToWorld(vdbRay_index.dir()).length();

        // Walk along the ray using DDA
        using RayT = openvdb::math::Ray<Float>;
        using TimeSpanT = typename RayT::TimeSpan;
        using TreeT = typename GridT::TreeType;
        using AccessorT = typename openvdb::tree::ValueAccessor<const TreeT, false>;
        constexpr int NodeLevel = TreeT::RootNodeType::ChildNodeType::LEVEL;
        openvdb::math::VolumeHDDA<TreeT, RayT, NodeLevel> dda;
        AccessorT accessor(grid_->constTree());
        bool done = false;
        while (!done) {
            // Next span of the grid
            const TimeSpanT ts = dda.march(vdbRay_index, accessor);
            if (!ts.valid()) {
                break;
            }

            // Convert the span in the world space
            const auto t0_w = length_indexToWorld * ts.t0;
            const auto t1_w = length_indexToWorld * ts.t1;

            // March along the ray with marchStep
            for (Float t = marchStep * std::ceil(t0_w / marchStep); t <= t1_w; t += marchStep) {
                // Position in the world space
                const auto p = vdbRay_world(t);
                if (!rayMarchFunc(toLMVec3(p))) {
                    done = true;
                    break;
                }
            }

            // Next ray
            vdbRay_index.setTimes(ts.t1 + openvdb::math::Delta<Float>::value(), tmax_index);
        }
    }
};

LM_COMP_REG_IMPL(Volume_OpenVDBScalar_, "volume::openvdb_scalar_");

LM_NAMESPACE_END(LM_NAMESPACE)
