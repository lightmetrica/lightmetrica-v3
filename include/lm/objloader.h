/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "math.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(objloader)

/*!
    \addtogroup objloader
    @{
*/

/*!
    \brief Surface geometry shared among meshes.
*/
struct OBJSurfaceGeometry {
    std::vector<Vec3> ps;       //!< Positions.
    std::vector<Vec3> ns;       //!< Normals.
    std::vector<Vec2> ts;       //!< Texture coordinates.

    template <typename Archive>
    void serialize(Archive& ar) {
        ar(ps, ns, ts);
    }
};

/*!
    \brief Face indices.
*/
struct OBJMeshFaceIndex {
    int p = -1;         //!< Index of position.
    int t = -1;         //!< Index of texture coordinates.
    int n = -1;         //!< Index of normal.

    template <typename Archive>
    void serialize(Archive& ar) {
        ar(p, t, n);
    }
};

/*!
    \brief Face.
*/
using OBJMeshFace = std::vector<OBJMeshFaceIndex>;

/*!
    \brief MLT material parameters.
*/
struct MTLMatParams {
    std::string name;   //!< Name.
    int illum;          //!< Type.
    Vec3 Kd;            //!< Diffuse reflectance.
    Vec3 Ks;            //!< Specular reflectance.
    Vec3 Ke;            //!< Luminance.
    std::string mapKd;  //!< Path to the texture.
    Float Ni;           //!< Index of refraction.
    Float Ns;           //!< Specular exponent for phong shading.
    Float an;           //!< Anisotropy.

    template <typename Archive>
    void serialize(Archive& ar) {
        ar(name, illum, Kd, Ks, Ke, mapKd, Ni, Ns, an);
    }
};

/*!
    \brief Callback function to process a mesh.
*/
using ProcessMeshFunc = std::function<bool(const OBJMeshFace& fs, const MTLMatParams& m)>;

/*!
    \brief Callback function to process a material.
*/
using ProcessMaterialFunc = std::function<bool(const MTLMatParams& m)>;

// ----------------------------------------------------------------------------

//! Default parallel context type
constexpr const char* DefaultType = "objloader::simple";

/*!
    \brief Initialize objloader context.
*/
LM_PUBLIC_API void init(const std::string& type = DefaultType, const Json& prop = {});

/*!
    \brief Shutdown parallel context.
*/
LM_PUBLIC_API void shutdown();

/*!
    \brief Load an OBJ file.
*/
LM_PUBLIC_API bool load(
    const std::string& path,
    OBJSurfaceGeometry& geo,
    const ProcessMeshFunc& processMesh,
    const ProcessMaterialFunc& processMaterial);

// ----------------------------------------------------------------------------

/*!
    \brief OBJ loader context.
*/
class OBJLoaderContext : public Component {
public:
    virtual bool load(
        const std::string& path,
        OBJSurfaceGeometry& geo,
        const ProcessMeshFunc& processMesh,
        const ProcessMaterialFunc& processMaterial) = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(objloader)
LM_NAMESPACE_END(LM_NAMESPACE)
