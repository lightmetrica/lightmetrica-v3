/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/mesh.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

struct MeshFaceIndex {
    int p = -1;   // Reference to position
    int t = -1;   // Reference to texture coordinates
    int n = -1;   // Reference to normal
};

class Mesh_Raw final : public Mesh {
private:
    std::vector<Vec3> ps_;           // Positions
    std::vector<Vec3> ns_;           // Normals
    std::vector<Vec2> ts_;           // Texture coordinates
    std::vector<MeshFaceIndex> fs_;  // Faces

public:
    virtual bool construct(const Json& prop, Component* parent) override {
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
        for (int i = 0; i < fs[0].size(); i += 2) {
            fs_.push_back(MeshFaceIndex{ fs[0][i],fs[1][i],fs[2][i] });
        }
    }


};

LM_COMP_REG_IMPL(Mesh_Raw, "mesh::raw");

LM_NAMESPACE_END(LM_NAMESPACE)
