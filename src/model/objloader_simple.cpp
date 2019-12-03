/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/objloader.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE::objloader)

// Wavefront OBJ/MTL file parser
class OBJLoaderContext_Simple : public OBJLoaderContext {
public:
    // Material parameters
    std::vector<MTLMatParams> ms_;
    std::unordered_map<std::string, int> msmap_;

public:
    virtual bool load(
        const std::string& path,
        OBJSurfaceGeometry& geo,
        const ProcessMeshFunc& process_mesh,
        const ProcessMaterialFunc& process_material) override
    {
        ms_.clear();
        msmap_.clear();

        LM_INFO("Loading OBJ file [path='{}']", fs::path(path).filename().string());
        char l[4096], name[256];
        std::ifstream f(path);
        if (!f) {
            LM_ERROR("Missing OBJ file [path='{}']", path);
            return false;
        }

        // Active face indices and material index
        int currMaterialIdx = -1;
        std::vector<OBJMeshFaceIndex> currfs;

        // Parse .obj file line by line
        while (f.getline(l, 4096)) {
            char *t = l;
            skip_spaces(t);
            if (command(t, "v", 1)) {
                geo.ps.emplace_back(next_vec3(t += 2));
            } else if (command(t, "vn", 2)) {
                geo.ns.emplace_back(next_vec3(t += 3));
            } else if (command(t, "vt", 2)) {
                geo.ts.emplace_back(next_vec3(t += 3));
            } else if (command(t, "f", 1)) {
                t += 2;
                if (ms_.empty()) {
                    // Process the case where MTL file is missing
                    ms_.push_back({ "default", -1, Vec3(1) });
                    if (!process_material(ms_.back())) {
                        return false;
                    }
                    currMaterialIdx = 0;
                }
                OBJMeshFaceIndex is[4];
                for (auto& i : is) {
                    if (eol(t[0])) { continue; }
                    i = parse_indices(geo, t);
                }
                currfs.insert(currfs.end(), {is[0], is[1], is[2]});
                if (is[3].p != -1) {
                    // Triangulate quad
                    currfs.insert(currfs.end(), {is[0], is[2], is[3]});
                }
            } else if (command(t, "usemtl", 6)) {
                t += 7;
                next_string(t, name);
                if (!currfs.empty()) {
                    // 'usemtl' indicates end of mesh groups
                    if (!process_mesh(currfs, ms_.at(currMaterialIdx))) {
                        return false;
                    }
                    currfs.clear();
                }
                currMaterialIdx = msmap_.at(name);
            } else if (command(t, "mtllib", 6)) {
                next_string(t += 7, name);
                if (!loadmtl((fs::path(path).remove_filename() / name).string(), process_material)) {
                    return false;
                }
            } else {
                continue;
            }
        }
        if (!currfs.empty()) {
            if (!process_mesh(currfs, ms_.at(currMaterialIdx))) {
                return false;
            }
        }

        return true;
    }

private:
    // Checks end of line
    bool eol(char c) { return c == '\0'; }

    // Checks a character is space-like
    bool whitespace(char c) { return c == ' ' || c == '\t'; };

    // Checks the token is a command 
    bool command(char *&t, const char *c, int n) { return !strncmp(t, c, n) && whitespace(t[n]); }

    // Skips spaces
    void skip_spaces(char *&t) { t += strspn(t, " \t"); }

    // Skips spaces or /
    void skip_spaces_or_comments(char *&t) { t += strcspn(t, "/ \t"); }

    // Parses floating point value
    Float next_float(char *&t) {
        skip_spaces(t);
        Float v = Float(atof(t));
        skip_spaces_or_comments(t);
        return v;
    }

    // Parses int value
    int next_int(char *&t) {
        skip_spaces(t);
        int v = atoi(t);
        skip_spaces_or_comments(t);
        return v;
    }

    // Parses 3d vector
    Vec3 next_vec3(char *&t) {
        Vec3 v;
        v.x = next_float(t);
        v.y = next_float(t);
        v.z = next_float(t);
        return v;
    }

    // Parses vertex index. See specification of obj file for detail.
    int parse_index(int i, int vn) { return i < 0 ? vn + i : i > 0 ? i - 1 : -1; }
    OBJMeshFaceIndex parse_indices(OBJSurfaceGeometry& geo, char *&t) {
        OBJMeshFaceIndex i;
        skip_spaces(t);
        i.p = parse_index(atoi(t), int(geo.ps.size()));
        skip_spaces_or_comments(t);
        if (eol(t[0]) || t++[0] != '/') { return i; }
        i.t = parse_index(atoi(t), int(geo.ts.size()));
        skip_spaces_or_comments(t);
        if (eol(t[0]) || t++[0] != '/') { return i; }
        i.n = parse_index(atoi(t), int(geo.ns.size()));
        skip_spaces_or_comments(t);
        return i;
    }

    // Parses a string
    template <int N>
    void next_string(char *&t, char (&name)[N]) {
        #if LM_COMPILER_MSVC
        sscanf_s(t, "%s", name, N);
        #else
        sscanf(t, "%s", name);
        #endif
    };

    // Parses .mtl file
    bool loadmtl(std::string p, const ProcessMaterialFunc& process_material) {
        LM_INFO("Loading MTL file [path='{}']", fs::path(p).filename().string());
        std::ifstream f(p);
        if (!f) {
            LM_ERROR("Missing MLT file [path='{}']", p);
            return false;
        }
        char l[4096], name[256];
        while (f.getline(l, 4096)) {
            auto *t = l;
            skip_spaces(t);
            if (command(t, "newmtl", 6)) {
                next_string(t += 7, name);
                msmap_[name] = int(ms_.size());
                ms_.emplace_back();
                ms_.back().name = name;
                continue;
            }
            if (ms_.empty()) {
                continue;
            }
            auto& m = ms_.back();
            if      (command(t, "Kd", 2))     { m.Kd = next_vec3(t += 3); }
            else if (command(t, "Ks", 2))     { m.Ks = next_vec3(t += 3); }
            else if (command(t, "Ni", 2))     { m.Ni = next_float(t += 3); }
            else if (command(t, "Ns", 2))     { m.Ns = next_float(t += 3); }
            else if (command(t, "aniso", 5))  { m.an = next_float(t += 5); }
            else if (command(t, "Ke", 2))     { m.Ke = next_vec3(t += 3); }
            else if (command(t, "illum", 5))  { m.illum = next_int(t += 6); }
            else if (command(t, "map_Kd", 6)) { next_string(t += 7, name); m.mapKd = name; }
        }
        // Let the user to process materials
        for (const auto& m : ms_) {
            if (!process_material(m)) {
                return false;
            }
        }
        return true;
    }
};

LM_COMP_REG_IMPL(OBJLoaderContext_Simple, "objloader::simple");

LM_NAMESPACE_END(LM_NAMESPACE::objloader)
