/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/objloader.h>
#include <lm/logger.h>

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
        const ProcessMeshFunc& processMesh,
        const ProcessMaterialFunc& processMaterial) override
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
            skipSpaces(t);
            if (command(t, "v", 1)) {
                geo.ps.emplace_back(nextVec3(t += 2));
            } else if (command(t, "vn", 2)) {
                geo.ns.emplace_back(nextVec3(t += 3));
            } else if (command(t, "vt", 2)) {
                geo.ts.emplace_back(nextVec3(t += 3));
            } else if (command(t, "f", 1)) {
                t += 2;
                if (ms_.empty()) {
                    // Process the case where MTL file is missing
                    ms_.push_back({ "default", -1, Vec3(1) });
                    if (!processMaterial(ms_.back())) {
                        return false;
                    }
                    currMaterialIdx = 0;
                }
                OBJMeshFaceIndex is[4];
                for (auto& i : is) {
                    if (eol(t[0])) { continue; }
                    i = parseIndices(geo, t);
                }
                currfs.insert(currfs.end(), {is[0], is[1], is[2]});
                if (is[3].p != -1) {
                    // Triangulate quad
                    currfs.insert(currfs.end(), {is[0], is[2], is[3]});
                }
            } else if (command(t, "usemtl", 6)) {
                t += 7;
                nextString(t, name);
                if (!currfs.empty()) {
                    // 'usemtl' indicates end of mesh groups
                    if (!processMesh(currfs, ms_.at(currMaterialIdx))) {
                        return false;
                    }
                    currfs.clear();
                }
                currMaterialIdx = msmap_.at(name);
            } else if (command(t, "mtllib", 6)) {
                nextString(t += 7, name);
                if (!loadmtl((fs::path(path).remove_filename() / name).string(), processMaterial)) {
                    return false;
                }
            } else {
                continue;
            }
        }
        if (!currfs.empty()) {
            if (!processMesh(currfs, ms_.at(currMaterialIdx))) {
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
    void skipSpaces(char *&t) { t += strspn(t, " \t"); }

    // Skips spaces or /
    void skipSpacesOrComments(char *&t) { t += strcspn(t, "/ \t"); }

    // Parses floating point value
    Float nextFloat(char *&t) {
        skipSpaces(t);
        Float v = Float(atof(t));
        skipSpacesOrComments(t);
        return v;
    }

    // Parses int value
    int nextInt(char *&t) {
        skipSpaces(t);
        int v = atoi(t);
        skipSpacesOrComments(t);
        return v;
    }

    // Parses 3d vector
    Vec3 nextVec3(char *&t) {
        Vec3 v;
        v.x = nextFloat(t);
        v.y = nextFloat(t);
        v.z = nextFloat(t);
        return v;
    }

    // Parses vertex index. See specification of obj file for detail.
    int parseIndex(int i, int vn) { return i < 0 ? vn + i : i > 0 ? i - 1 : -1; }
    OBJMeshFaceIndex parseIndices(OBJSurfaceGeometry& geo, char *&t) {
        OBJMeshFaceIndex i;
        skipSpaces(t);
        i.p = parseIndex(atoi(t), int(geo.ps.size()));
        skipSpacesOrComments(t);
        if (eol(t[0]) || t++[0] != '/') { return i; }
        i.t = parseIndex(atoi(t), int(geo.ts.size()));
        skipSpacesOrComments(t);
        if (eol(t[0]) || t++[0] != '/') { return i; }
        i.n = parseIndex(atoi(t), int(geo.ns.size()));
        skipSpacesOrComments(t);
        return i;
    }

    // Parses a string
    template <int N>
    void nextString(char *&t, char (&name)[N]) {
        #if LM_COMPILER_MSVC
        sscanf_s(t, "%s", name, N);
        #else
        sscanf(t, "%s", name);
        #endif
    };

    // Parses .mtl file
    bool loadmtl(std::string p, const ProcessMaterialFunc& processMaterial) {
        LM_INFO("Loading MTL file [path='{}']", fs::path(p).filename().string());
        std::ifstream f(p);
        if (!f) {
            LM_ERROR("Missing MLT file [path='{}']", p);
            return false;
        }
        char l[4096], name[256];
        while (f.getline(l, 4096)) {
            auto *t = l;
            skipSpaces(t);
            if (command(t, "newmtl", 6)) {
                nextString(t += 7, name);
                msmap_[name] = int(ms_.size());
                ms_.emplace_back();
                ms_.back().name = name;
                continue;
            }
            if (ms_.empty()) {
                continue;
            }
            auto& m = ms_.back();
            if      (command(t, "Kd", 2))     { m.Kd = nextVec3(t += 3); }
            else if (command(t, "Ks", 2))     { m.Ks = nextVec3(t += 3); }
            else if (command(t, "Ni", 2))     { m.Ni = nextFloat(t += 3); }
            else if (command(t, "Ns", 2))     { m.Ns = nextFloat(t += 3); }
            else if (command(t, "aniso", 5))  { m.an = nextFloat(t += 5); }
            else if (command(t, "Ke", 2))     { m.Ke = nextVec3(t += 3); }
            else if (command(t, "illum", 5))  { m.illum = nextInt(t += 6); }
            else if (command(t, "map_Kd", 6)) { nextString(t += 7, name); m.mapKd = name; }
        }
        // Let the user to process materials
        for (const auto& m : ms_) {
            if (!processMaterial(m)) {
                return false;
            }
        }
        return true;
    }
};

LM_COMP_REG_IMPL(OBJLoaderContext_Simple, "objloader::simple");

LM_NAMESPACE_END(LM_NAMESPACE::objloader)
