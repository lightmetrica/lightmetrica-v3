/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>
#ifdef __GNUC__
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif
#define TINYOBJLOADER_USE_DOUBLE
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE::objloader)

class OBJLoaderContext_TinyOBJLoader : public OBJLoaderContext {
public:
    virtual bool load(
        const std::string& path,
        OBJSurfaceGeometry& geo,
        const ProcessMeshFunc& process_mesh,
        const ProcessMaterialFunc& process_material) override
    {
        exception::ScopedDisableFPEx guard_;

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn;
        std::string err;

        // Load OBJ file
        const auto basedir = fs::path(path).parent_path().string();
        const bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), basedir.c_str(), true);
        if (!err.empty()) {
            LM_ERROR(err);
        }
        if (!warn.empty()) {
            LM_WARN(warn);
        }
        if (!ret) {
            return false;
        }

        // Copy vertex data
        // This incurs temporal double allocation of the vertex buffers.
        geo.ps.assign(attrib.vertices.size() / 3, Vec3());
        memcpy(&geo.ps[0].x, &attrib.vertices[0], sizeof(Float) * attrib.vertices.size());
        attrib.vertices.clear();
        attrib.vertices.shrink_to_fit();
        if (!attrib.normals.empty()) {
            geo.ns.assign(attrib.normals.size() / 3, Vec3());
            memcpy(&geo.ns[0].x, &attrib.normals[0], sizeof(Float) * attrib.normals.size());
            attrib.normals.clear();
            attrib.normals.shrink_to_fit();
        }
        if (!attrib.texcoords.empty()) {
            geo.ts.assign(attrib.texcoords.size() / 2, Vec2());
            memcpy(&geo.ts[0].x, &attrib.texcoords[0], sizeof(Float) * attrib.texcoords.size());
            attrib.texcoords.clear();
            attrib.texcoords.shrink_to_fit();
        }

        // Process materials
        std::vector<MTLMatParams> our_mat_params;
        for (const auto& mat : materials) {
            // Convert to our type
            MTLMatParams p;
            p.name = mat.name;
            p.illum = mat.illum;
            p.Kd = Vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
            p.Ks = Vec3(mat.specular[0], mat.specular[1], mat.specular[2]);
            p.Ke = Vec3(mat.emission[0], mat.emission[1], mat.emission[2]);
            p.mapKd = mat.diffuse_texname;
            p.Ni = mat.ior;
            p.Ns = mat.shininess;
            p.an = mat.anisotropy;

            // Process material
            if (!process_material(p)) {
                return false;
            }

            // We need the converted parameters later
            our_mat_params.push_back(std::move(p));
        }

        // Default material if OBJ has no corresponding MTL file
        if (materials.empty()) {
            MTLMatParams p;
            p.name = "default";
            p.illum = -1;
            p.Kd = Vec3(1);
            if (!process_material(p)) {
                return false;
            }
            our_mat_params.push_back(std::move(p));
        }

        // Process shapes
        OBJMeshFace fs;
        for (const auto& shape : shapes) {
            // Separate the shape according to the materials,
            // because our framework doesn't support per-face material.
            fs.clear();
            const int num_faces = int(shape.mesh.num_face_vertices.size());
            int prev_material_id = shape.mesh.material_ids[0];
            for (int fi = 0; fi < num_faces; fi++) {
                // Assume the mesh is triangulated
                if (shape.mesh.num_face_vertices[fi] != 3) {
                    LM_ERROR("A mesh must be triangulated");
                    return false;
                }
                
                // If the material changes, create a mesh
                // with current indices and previous material
                const int curr_material_id = shape.mesh.material_ids[fi];
                if (curr_material_id != prev_material_id) {
                    if (!process_mesh(fs, our_mat_params[prev_material_id < 0 ? 0 : prev_material_id])) {
                        return false;
                    }
                    fs.clear();
                    prev_material_id = curr_material_id;
                }
                else {
                    const auto& i1 = shape.mesh.indices[fi*3];
                    const auto& i2 = shape.mesh.indices[fi*3+1];
                    const auto& i3 = shape.mesh.indices[fi*3+2];
                    fs.push_back({ i1.vertex_index, i1.texcoord_index, i1.normal_index });
                    fs.push_back({ i2.vertex_index, i2.texcoord_index, i2.normal_index });
                    fs.push_back({ i3.vertex_index, i3.texcoord_index, i3.normal_index });
                }
            }
            if (!fs.empty()) {
                if (!process_mesh(fs, our_mat_params[prev_material_id < 0 ? 0 : prev_material_id])) {
                    return false;
                }
            }
        }

        return true;
    }
};

LM_COMP_REG_IMPL(OBJLoaderContext_TinyOBJLoader, "objloader::tinyobjloader");

LM_NAMESPACE_END(LM_NAMESPACE::objloader)
