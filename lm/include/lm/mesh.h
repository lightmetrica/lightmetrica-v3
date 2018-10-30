/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "detail/component.h"
#include <vector>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

struct MeshGeometry {
    std::vector<vec3> ps;   // Positions
    std::vector<vec3> ns;   // Normals
    std::vector<vec2> ts;   // Texture coordinates
};

struct MeshFaceIndex {
    int p = -1;   // Reference to position
    int t = -1;   // Reference to texture coordinates
    int n = -1;   // Reference to normal
};

struct Mesh {
    std::vector<MeshFaceIndex> fs;  // Faces
    const MeshGeometry* geo;        // Reference to geometry
};

class Model : public Component {
public:
    

};

LM_NAMESPACE_END(LM_NAMESPACE)
