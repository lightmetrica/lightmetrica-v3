/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/mesh.h>
#include <lm/objloader.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
\rst
.. function:: mesh::wavefrontobj

    Mesh for Wavefront OBJ format. 
    
    This asset considers the geometry contained in the waverfont OBJ as as a single mesh asset.
    If you want to use multiple meshes, please try ``model::wavefrontobj``.
\endrst
*/
class Mesh_WavefrontObj final : public Mesh {
private:
    objloader::OBJSurfaceGeometry geo_;
    objloader::OBJMeshFace fs_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(geo_, fs_);
    }

public:
    virtual void construct(const Json& prop) override {
        using namespace objloader;
        const std::string path = json::value<std::string>(prop, "path");
        const bool result = objloader::load(path, geo_,
            [&](const OBJMeshFace& fs, const MTLMatParams&) -> bool {
                // Accumulate face index
                fs_.insert(fs_.end(), fs.begin(), fs.end());
                return true;
            },
            [&](const MTLMatParams&) -> bool {
                // Ignore materials
                return true;
            });
        if (!result) {
            LM_THROW_EXCEPTION_DEFAULT(Error::IOError);
        }
    }

    virtual void foreach_triangle(const ProcessTriangleFunc& process_triangle) const override {
        for (int fi = 0; fi < int(fs_.size())/3; fi++) {
            process_triangle(fi, triangle_at(fi));
        }
    }

    virtual Tri triangle_at(int face) const override {
        const auto f1 = fs_[3*face];
        const auto f2 = fs_[3*face+1];
        const auto f3 = fs_[3*face+2];
        return {
            { geo_.ps[f1.p], f1.n<0 ? Vec3() : geo_.ns[f1.n], f1.t<0 ? Vec2() : geo_.ts[f1.t] },
            { geo_.ps[f2.p], f2.n<0 ? Vec3() : geo_.ns[f2.n], f2.t<0 ? Vec2() : geo_.ts[f2.t] },
            { geo_.ps[f3.p], f3.n<0 ? Vec3() : geo_.ns[f3.n], f3.t<0 ? Vec2() : geo_.ts[f3.t] }
        };
    }

    virtual InterpolatedPoint surface_point(int face, Vec2 uv) const override {
        const auto i1 = fs_[3*face];
        const auto i2 = fs_[3*face+1];
        const auto i3 = fs_[3*face+2];
        const auto p1 = geo_.ps[i1.p];
        const auto p2 = geo_.ps[i2.p];
        const auto p3 = geo_.ps[i3.p];
        return {
            // Position
            math::mix_barycentric(p1, p2, p3, uv),
            // Normal. Use geometry normal if the attribute is missing.
            i1.n < 0 ? math::geometry_normal(p1, p2, p3)
                     : glm::normalize(math::mix_barycentric(
                         geo_.ns[i1.n], geo_.ns[i2.n], geo_.ns[i3.n], uv)),
            // Geometry normal
            math::geometry_normal(p1, p2, p3),
            // Texture coordinates
            i1.t < 0 ? Vec2(0) : math::mix_barycentric(
                geo_.ts[i1.t], geo_.ts[i2.t], geo_.ts[i3.t], uv)
        };
    }

    virtual int num_triangles() const override {
        return int(fs_.size()) / 3;
    }
};

LM_COMP_REG_IMPL(Mesh_WavefrontObj, "mesh::wavefrontobj");

LM_NAMESPACE_END(LM_NAMESPACE)
