/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/mesh.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

struct MeshFaceIndex {
    int p = -1;   // Index of position
    int t = -1;   // Index of texture coordinates
    int n = -1;   // Index of normal

    template <typename Archive>
    void serialize(Archive& ar) {
        ar(p, t, n);
    }
};

/*
\rst
.. function:: mesh::raw

    Mesh from raw data.

    :param list ps: Vertex positions of the mesh.
    :param list ns: Vertex normals of the mesh.
    :param list ts: Texture coordinates for the vertices.
    :param dist fs: Index list. Indices for each vertex element are
                    specified by ``p``, ``t``, and ``n`` respectively.

\endrst
*/
class Mesh_Raw final : public Mesh {
private:
    std::vector<Vec3> ps_;           // Positions
    std::vector<Vec3> ns_;           // Normals
    std::vector<Vec2> ts_;           // Texture coordinates
    std::vector<MeshFaceIndex> fs_;  // Faces

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(ps_, ns_, ts_, fs_);
    }

public:
    virtual void construct(const Json& prop) override {
        const auto& ps = prop["ps"];
        for (int i = 0; i < ps.size(); i+=3) {
            ps_.push_back(Vec3(ps[i],ps[i+1],ps[i+2]));
        }
        const auto& ns = prop["ns"];
        for (int i = 0; i < ns.size(); i+=3) {
            ns_.push_back(Vec3(ns[i],ns[i+1],ns[i+2]));
        }
        const auto& ts = prop["ts"];
        for (int i = 0; i < ts.size(); i+=2) {
            ts_.push_back(Vec2(ts[i],ts[i+1]));
        }
        const auto& fs = prop["fs"];
        const int fs_size = int(fs["p"].size());
        for (int i = 0; i < fs_size; i++) {
            fs_.push_back(MeshFaceIndex{ fs["p"][i],fs["t"][i],fs["n"][i] });
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
            { ps_[f1.p], ns_[f1.n], ts_[f1.t] },
            { ps_[f2.p], ns_[f2.n], ts_[f2.t] },
            { ps_[f3.p], ns_[f3.n], ts_[f3.t] }
        };
    }

    virtual InterpolatedPoint surface_point(int face, Vec2 uv) const override {
        const auto i1 = fs_[3*face];
        const auto i2 = fs_[3*face+1];
        const auto i3 = fs_[3*face+2];
        return {
            math::mix_barycentric(ps_[i1.p], ps_[i2.p], ps_[i3.p], uv),
            glm::normalize(math::mix_barycentric(ns_[i1.n], ns_[i2.n], ns_[i3.n], uv)),
            math::geometry_normal(ps_[i1.p], ps_[i2.p], ps_[i3.p]),
            math::mix_barycentric(ts_[i1.t], ts_[i2.t], ts_[i3.t], uv)
        };
    }

    virtual int num_triangles() const override {
        return int(fs_.size()) / 3;
    }
};

LM_COMP_REG_IMPL(Mesh_Raw, "mesh::raw");

LM_NAMESPACE_END(LM_NAMESPACE)
