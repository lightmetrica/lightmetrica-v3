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

class Mesh_Raw : public Mesh {
private:
    std::vector<MeshFaceIndex> fs;  // Faces
    std::vector<vec3> ps;   // Positions
    std::vector<vec3> ns;   // Normals
    std::vector<vec2> ts;   // Texture coordinates

public:
    virtual bool construct(const json& prop, Component* parent) override {
        
    }
};

LM_COMP_REG_IMPL(Mesh_Raw, "mesh::raw");

LM_NAMESPACE_END(LM_NAMESPACE)
