/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include "pch.h"
//#include <lm/detail/component.h>
//#include <lm/math.h>
//
//LM_NAMESPACE_BEGIN(LM_NAMESPACE)
//
//// ----------------------------------------------------------------------------
//
//// Wavefront OBJ file parser
//// Basic material parameters (extracted from .mtl file)
//struct MatParams {
//    int illum;               // Type
//    vec3 Kd;                    // Diffuse reflectance
//    vec3 Ks;                    // Specular reflectance
//    vec3 Ke;                    // Luminance
//    Tex<F> *mapKd = nullptr; // Texture for Kd
//    F Ni;                    // Index of refraction
//    F Ns;                    // Specular exponent for phong shading
//    F an;                    // Anisotropy
//};
//
//class WavefrontOBJParser {
//private:
//    // Material parameters
//    std::vector<MatParams> ms;
//    std::unordered_map<std::string, int> msmap;  // Map of material name
//    std::unordered_map<std::string, int> tsmap;  // Map of texture name
//
//    // References
//    Geo<F>& geo;
//    std::vector<std::unique_ptr<Tex<F>>>& ts;
//
//public:
//    WavefrontOBJParser(Geo<F>& geo, std::vector<std::unique_ptr<Tex<F>>>& ts)
//        : geo(geo), ts(ts) {}
//
//    // Parses .obj file
//    void parse(std::string p, const std::function<void(const MatParams<F>& m, const std::vector<Ind>& fs)>& processMesh) {
//        char l[4096], name[256];
//        std::ifstream f(p);
//        std::cout << "Loading OBJ file: " << p << std::endl;
//
//        // Active face indices and material index
//        int currMaterialIdx = -1;
//        std::vector<Ind> currfs;
//
//        // Parse .obj file line by line
//        while (f.getline(l, 4096)) {
//            char *t = l;
//            ss(t);
//            if (cm(t, "v", 1)) {
//                geo.ps.emplace_back(nv(t += 2));
//            } else if (cm(t, "vn", 2)) {
//                geo.ns.emplace_back(nv(t += 3));
//            } else if (cm(t, "vt", 2)) {
//                geo.ts.emplace_back(nv(t += 3));
//            } else if (cm(t, "f", 1)) {
//                t += 2;
//                if (ms.empty()) {
//                    ms.push_back({ -1, V(1) });  // NonS
//                    currMaterialIdx = 0;
//                }
//                Ind is[4];
//                for (auto& i : is) {
//                    i = pind(t);
//                }
//                currfs.insert(currfs.end(), {is[0], is[1], is[2]});
//                if (is[3].p != -1) {
//                    currfs.insert(currfs.end(), {is[0], is[2], is[3]});
//                }
//            } else if (cm(t, "usemtl", 6)) {
//                t += 7;
//                nn(t, name);
//                if (!currfs.empty()) {
//                    processMesh(ms[currMaterialIdx], currfs);
//                    currfs.clear();
//                }
//                currMaterialIdx = msmap.at(name);
//            } else if (cm(t, "mtllib", 6)) {
//                nn(t += 7, name);
//                loadmtl(sanitizeSeparator((std::filesystem::path(p).remove_filename() / name).string()), ts);
//            } else {
//                continue;
//            }
//        }
//        if (!currfs.empty()) {
//            processMesh(ms[currMaterialIdx], currfs);
//        }
//    }
//
//private:
//    // Checks a character is space-like
//    bool ws(char c) { return c == ' ' || c == '\t'; };
//
//    // Checks the token is a command 
//    bool cm(char *&t, const char *c, int n) { return !strncmp(t, c, n) && ws(t[n]); }
//
//    // Skips spaces
//    void ss(char *&t) { t += strspn(t, " \t"); }
//
//    // Skips spaces or /
//    void sc(char *&t) { t += strcspn(t, "/ \t"); }
//
//    // Parses floating point value
//    F nf(char *&t) {
//        ss(t);
//        F v = F(atof(t));
//        sc(t);
//        return v;
//    }
//
//    // Parses int value
//    int ni(char *&t) {
//        ss(t);
//        int v = atoi(t);
//        sc(t);
//        return v;
//    }
//
//    // Parses 3d vector
//    V nv(char *&t) {
//        V v;
//        v.x = nf(t);
//        v.y = nf(t);
//        v.z = nf(t);
//        return v;
//    }
//
//    // Parses vertex index. See specification of obj file for detail.
//    int pi(int i, int vn) { return i < 0 ? vn + i : i > 0 ? i - 1 : -1; }
//    Ind pind(char *&t) {
//        Ind i;
//        ss(t);
//        i.p = pi(atoi(t), int(geo.ps.size()));
//        sc(t);
//        if (t++[0] != '/') { return i; }
//        i.t = pi(atoi(t), int(geo.ts.size()));
//        sc(t);
//        if (t++[0] != '/') { return i; }
//        i.n = pi(atoi(t), int(geo.ns.size()));
//        sc(t);
//        return i;
//    }
//
//    // Parses a string
//    void nn(char *&t, char name[]) { sscanf(t, "%s", name); };
//
//    // Parses .mtl file
//    void loadmtl(std::string p, std::vector<std::unique_ptr<Tex<F>>>& ts) {
//        std::ifstream f(p);
//        char l[4096], name[256];
//        std::cout << "Loading MTL file: " << p << std::endl;
//        while (f.getline(l, 4096)) {
//            auto *t = l;
//            ss(t);
//            if (cm(t, "newmtl", 6)) {
//                nn(t += 7, name);
//                msmap[name] = int(ms.size());
//                ms.emplace_back();
//                continue;
//            }
//            if (ms.empty()) {
//                continue;
//            }
//            auto& m = ms.back();
//            if      (cm(t, "Kd", 2))     { m.Kd = nv(t += 3); }
//            else if (cm(t, "Ks", 2))     { m.Ks = nv(t += 3); }
//            else if (cm(t, "Ni", 2))     { m.Ni = nf(t += 3); }
//            else if (cm(t, "Ns", 2))     { m.Ns = nf(t += 3); }
//            else if (cm(t, "aniso", 5))  { m.an = nf(t += 5); }
//            else if (cm(t, "Ke", 2))     {
//                m.Ke = nv(t += 3);
//            }
//            else if (cm(t, "illum", 5))  { m.illum = ni(t += 6); }
//            else if (cm(t, "map_Kd", 6)) {
//                nn(t += 7, name);
//                auto it = tsmap.find(name);
//                if (it != tsmap.end()) {
//                    m.mapKd = ts[it->second].get();
//                    continue;
//                }
//                tsmap[name] = int(ts.size());
//                ts.emplace_back(new Tex<F>);
//                ts.back()->load(sanitizeSeparator((std::filesystem::path(p).remove_filename() / name).string()));
//                m.mapKd = ts.back().get();
//            }
//            else {
//                continue;
//            }
//        }
//    }
//};
//
//// ----------------------------------------------------------------------------
//
//class Model_WavefrontObj : public Component {
//public:
//    virtual bool construct(const json& prop, Component* parent) {
//        
//    }
//
//};
//
//LM_NAMESPACE_END(LM_NAMESPACE)
