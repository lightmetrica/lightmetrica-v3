/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/model.h>
#include <lm/mesh.h>
#include <lm/material.h>
#include <lm/texture.h>
#include <lm/logger.h>
#include <lm/film.h>
#include <lm/light.h>
#include <lm/scene.h>
#include <lm/json.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// ----------------------------------------------------------------------------

// Surface geometry shared among meshes
struct OBJSurfaceGeometry {
    std::vector<Vec3> ps;   // Positions
    std::vector<Vec3> ns;   // Normals
    std::vector<Vec2> ts;   // Texture coordinates
};

// Face indices
struct OBJMeshFaceIndex {
    int p = -1;         // Index of position
    int t = -1;         // Index of texture coordinates
    int n = -1;         // Index of normal
};

// Texture parameters
struct MTLTextureParams {
    std::string name;   // Name
    std::string path;   // Texture path
};

// MLT material parameters
struct MTLMatParams {
    std::string name;   // Name
    int illum;          // Type
    Vec3 Kd;            // Diffuse reflectance
    Vec3 Ks;            // Specular reflectance
    Vec3 Ke;            // Luminance
    int mapKd = -1;     // Texture index for Kd
    Float Ni;           // Index of refraction
    Float Ns;           // Specular exponent for phong shading
    Float an;           // Anisotropy
};

// Wavefront OBJ/MTL file parser
class WavefrontOBJParser {
private:
    // Material parameters
    std::vector<MTLMatParams> ms_;
    std::unordered_map<std::string, int> msmap_;
    // Texture parameters
    std::vector<MTLTextureParams> ts_;
    std::unordered_map<std::string, int> tsmap_;

public:
    // Callback functions
    using ProcessMeshFunc = std::function<
        std::optional<int>(const std::vector<OBJMeshFaceIndex>& fs, const MTLMatParams& m)>;
    using ProcessMaterialFunc = std::function<
        bool(const MTLMatParams& m)>;
    using ProcessTextureFunc = std::function<
        bool(const MTLTextureParams& path)>;

    // Parses .obj file
    bool parse(const std::string& p, OBJSurfaceGeometry& geo,
        const ProcessMeshFunc& processMesh,
        const ProcessMaterialFunc& processMaterial,
        const ProcessTextureFunc& processTexture)
    {
        LM_INFO("Loading OBJ file [path='{}']", p);
        char l[4096], name[256];
        std::ifstream f(p);
        if (!f) {
            LM_ERROR("Missing OBJ file [path='{}']", p);
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
                    processMesh(currfs, ms_.at(currMaterialIdx));
                    currfs.clear();
                }
                currMaterialIdx = msmap_.at(name);
            } else if (command(t, "mtllib", 6)) {
                nextString(t += 7, name);
                if (!loadmtl((std::filesystem::path(p).remove_filename() / name).string(),
                    processMaterial, processTexture)) {
                    return false;
                }
            } else {
                continue;
            }
        }
        if (!currfs.empty()) {
            processMesh(currfs, ms_.at(currMaterialIdx));
        }

        return true;
    }

private:
    // Checks a character is space-like
    bool whitespace(char c) { return c == ' ' || c == '\t'; };

    // Checks the token is a command 
    bool command(char *&t, const char *c, int n) { return !strncmp(t, c, n) && whitespace(t[n]); }

    // Skips spaces
    void skipSpaces(char *&t) { t += strspn(t, " \t"); }

    // Skips spaces or /
    void skipSpacesOrComments(char *&t) { t += strcspn(t, "/ \t"); }

    // Parses floating point value
    Float nf(char *&t) {
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
        v.x = nf(t);
        v.y = nf(t);
        v.z = nf(t);
        return v;
    }

    // Parses vertex index. See specification of obj file for detail.
    int parseIndex(int i, int vn) { return i < 0 ? vn + i : i > 0 ? i - 1 : -1; }
    OBJMeshFaceIndex parseIndices(OBJSurfaceGeometry& geo, char *&t) {
        OBJMeshFaceIndex i;
        skipSpaces(t);
        i.p = parseIndex(atoi(t), int(geo.ps.size()));
        skipSpacesOrComments(t);
        if (t++[0] != '/') { return i; }
        i.t = parseIndex(atoi(t), int(geo.ts.size()));
        skipSpacesOrComments(t);
        if (t++[0] != '/') { return i; }
        i.n = parseIndex(atoi(t), int(geo.ns.size()));
        skipSpacesOrComments(t);
        return i;
    }

    // Parses a string
    template <int N>
    void nextString(char *&t, char (&name)[N]) { sscanf_s(t, "%s", name, N); };

    // Parses .mtl file
    bool loadmtl(std::string p, const ProcessMaterialFunc& processMaterial, const ProcessTextureFunc& processTexture)
    {
        LM_INFO("Loading MTL file [path='{}']", p);
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
            else if (command(t, "Ni", 2))     { m.Ni = nf(t += 3); }
            else if (command(t, "Ns", 2))     { m.Ns = nf(t += 3); }
            else if (command(t, "aniso", 5))  { m.an = nf(t += 5); }
            else if (command(t, "Ke", 2))     { m.Ke = nextVec3(t += 3); }
            else if (command(t, "illum", 5))  { m.illum = nextInt(t += 6); }
            else if (command(t, "map_Kd", 6)) {
                nextString(t += 7, name);
                // Check if the texture is alread loaded
                auto it = tsmap_.find(name);
                if (it != tsmap_.end()) {
                    m.mapKd = it->second;
                }
                else {
                    // Register a new texture
                    m.mapKd = tsmap_[name] = int(ts_.size());
                    ts_.push_back({name, (std::filesystem::path(p).remove_filename()/name).string()});
                }
            }
        }
        // Let the user to process texture and materials
        for (const auto& t : ts_) {
            if (!processTexture(t)) {
                return false;
            }
        }
        for (const auto& m : ms_) {
            if (!processMaterial(m)) {
                return false;
            }
        }
        return true;
    }
};

// ----------------------------------------------------------------------------

class Mesh_WavefrontObj : public Mesh {
private:
    OBJSurfaceGeometry& geo_;
    std::vector<OBJMeshFaceIndex> fs_;

public:
    Mesh_WavefrontObj(OBJSurfaceGeometry& geo, const std::vector<OBJMeshFaceIndex>& fs)
        : geo_(geo), fs_(fs) {}
    
    virtual void foreachTriangle(const ProcessTriangleFunc& processTriangle) const override {
        for (int fi = 0; fi < int(fs_.size()); fi += 3) {
            const auto p1 = geo_.ps[fs_[fi].p];
            const auto p2 = geo_.ps[fs_[fi + 1].p];
            const auto p3 = geo_.ps[fs_[fi + 2].p];
            processTriangle(fi, p1, p2, p3);
        }
    }

    virtual Point surfacePoint(int face, Vec2 uv) const override {
        const auto i1 = fs_[face];
        const auto i2 = fs_[face + 1];
        const auto i3 = fs_[face + 2];
        const auto p1 = geo_.ps[i1.p];
        const auto p2 = geo_.ps[i2.p];
        const auto p3 = geo_.ps[i3.p];
        return {
            // Position
            math::mixBarycentric(p1, p2, p3, uv),
            // Normal. Use geometry normal if the attribute is missing.
            i1.n < 0 ? glm::normalize(glm::cross(p2-p1, p3-p1))
                     : glm::normalize(math::mixBarycentric(
                         geo_.ns[i1.n], geo_.ns[i2.n], geo_.ns[i3.n], uv)),
            // Texture coordinates
            i1.t < 0 ? Vec2(0) : math::mixBarycentric(
                geo_.ts[i1.t], geo_.ts[i2.t], geo_.ts[i3.t], uv)
        };
    }
};

// ----------------------------------------------------------------------------

class Material_WavefrontObj : public Material {
private:
    // Material parameters of MLT file
    MTLMatParams objmat_;

    // Underlying material components
    std::vector<Ptr<Material>> materials_;

    // Component indices
    int diffuse_ = -1;
    int glossy_ = -1;
    int glass_ = -1;
    int mirror_ = -1;
    int mask_ = -1;

    // Texture for the alpha mask
    const Texture* maskTex_ = nullptr;

public:
    Material_WavefrontObj(const MTLMatParams& m) : objmat_(m) {}

public:
    virtual Component* underlyingAt(int index) const {
        // Deligate to parent component
        return parent()->underlyingAt(index);
    }

    virtual bool construct(const Json& prop) override {
        // Underlying assets name
        const auto glassMaterialName   = valueOr<std::string>(prop, "glass", "material::glass");
        const auto mirrorMaterialName  = valueOr<std::string>(prop, "mirror", "material::mirror");
        const auto diffuseMaterialName = valueOr<std::string>(prop, "diffuse", "material::diffuse");
        const auto glossyMaterialName  = valueOr<std::string>(prop, "glossy", "material::glossy");

        // Make parent component as a parent for the newly created components
        if (objmat_.illum == 7) {
            // Glass material
            auto p = comp::create<Material>(glassMaterialName, this, {
                {"Ni", objmat_.Ni}
            });
            if (!p) {
                return false;
            }
            glass_ = int(materials_.size());
            materials_.push_back(std::move(p));
        }
        else if (objmat_.illum == 5) {
            // Mirror material
            auto p = comp::create<Material>(mirrorMaterialName, this);
            if (!p) {
                return false;
            }
            mirror_ = int(materials_.size());
            materials_.push_back(std::move(p));
        }
        else {
            // Diffuse material
            auto diffuse = comp::create<Material>(diffuseMaterialName, this, {
                {"Kd", objmat_.Kd},
                {"mapKd", objmat_.mapKd < 0 ? Json{} : objmat_.mapKd}
            });
            if (!diffuse) {
                return false;
            }
            diffuse_ = int(materials_.size());
            materials_.push_back(std::move(diffuse));

            // Glossy material
            const auto r = 2_f / (2_f + objmat_.Ns);
            const auto as = math::safeSqrt(1_f - objmat_.an * .9_f);
            auto glossy = comp::create<Material>(glossyMaterialName, this, {
                {"Ks", objmat_.Ks},
                {"ax", std::max(1e-3_f, r / as)},
                {"ay", std::max(1e-3_f, r * as)}
            });
            if (!glossy) {
                return false;
            }
            glossy_ = int(materials_.size());
            materials_.push_back(std::move(glossy));

            // Mask texture
            if (objmat_.mapKd >= 0) {
                if (const auto* texture = parent()->underlyingAt<Texture>(objmat_.mapKd); texture->hasAlpha()) {
                    auto mask = comp::create<Material>("material::mask", this);
                    maskTex_ = texture;
                    if (!mask || !maskTex_) {
                        return false;
                    }
                    mask_ = int(materials_.size());
                    materials_.push_back(std::move(mask));
                }
            }
        }
        return true;
    }

    virtual bool isSpecular(const SurfacePoint& sp) const override {
        return materials_.at(sp.comp)->isSpecular(sp);
    }

    virtual std::optional<RaySample> sampleRay(Rng& rng, const SurfacePoint& sp, Vec3 wi) const {
        if (glass_ >= 0 || mirror_ >= 0) {
            // Glass or mirror
            int comp = mirror_ < 0 ? glass_ : mirror_;
            auto s = materials_.at(comp)->sampleRay(rng, sp, wi);
            if (!s) { return {}; }
            return s->asComp(comp);
        }
        else {
            // Diffuse or glossy or mask
            const auto* diffuse = materials_.at(diffuse_).get();
            const auto* glossy  = materials_.at(glossy_).get();
            const auto wd = [&]() {
                const auto wd = glm::compMax(diffuse->reflectance(sp));
                const auto ws = glm::compMax(glossy->reflectance(sp));
                return wd == 0_f && ws == 0_f ? 1_f : wd / (wd + ws);
            }();
            if (rng.u() < wd) {
                if (maskTex_ && rng.u() > maskTex_->evalAlpha(sp.t)) { // Mask
                    auto s = materials_.at(mask_)->sampleRay(rng, sp, wi);
                    if (!s) { return {}; }
                    return s->asComp(mask_).multWeight(1_f/wd);
                }
                else { // Diffuse
                    auto s = diffuse->sampleRay(rng, sp, wi);
                    if (!s) { return {}; }
                    return s->asComp(diffuse_).multWeight(1_f/wd);
                }
            }
            else { // Glossy
                auto s = glossy->sampleRay(rng, sp, wi);
                if (!s) { return {}; }
                return s->asComp(glossy_).multWeight(1_f/(1_f-wd));
            }
        }
    }

    virtual Float pdf(const SurfacePoint& sp, Vec3 wi, Vec3 wo) const {
        return materials_.at(sp.comp)->pdf(sp, wi, wo);
    }

    virtual Vec3 eval(const SurfacePoint& sp, Vec3 wi, Vec3 wo) const {
        return materials_.at(sp.comp)->eval(sp, wi, wo);
    }
};

// ----------------------------------------------------------------------------

class Model_WavefrontObj : public Model {
private:
    // Surface geometry
    OBJSurfaceGeometry geo_;
    // Underlying assets
    std::vector<Ptr<Component>> assets_;
    std::unordered_map<std::string, int> assetsMap_;
    // Mesh group which assocites a mesh and a material
    std::vector<std::tuple<int, int, int>> groups_;

public:
    virtual Component* underlyingAt(int index) const {
        return assets_.at(index).get();
    }

    virtual bool construct(const Json& prop) override {
        WavefrontOBJParser parser;
        return parser.parse(prop["path"], geo_,
            // Process mesh
            [&](const std::vector<OBJMeshFaceIndex>& fs, const MTLMatParams& m) -> std::optional<int> {
                // Create mesh
                auto mesh = comp::detail::createDirect<Mesh_WavefrontObj>(this, geo_, fs);
                if (!mesh) {
                    return false;
                }
                int meshIndex = int(assets_.size());
                assets_.push_back(std::move(mesh));

                // Create area light if Ke > 0
                int lightIndex = -1;
                if (glm::compMax(m.Ke) > 0_f) {
                    auto light = comp::create<Light>("light::area", this, {
                        {"Ke", m.Ke},
                        {"mesh", meshIndex}
                    });
                    if (!light) {
                        return false;
                    }
                    lightIndex = int(assets_.size());
                    assets_.push_back(std::move(light));
                }

                // Create mesh group
                groups_.push_back({ meshIndex, assetsMap_[m.name], lightIndex });

                return true;
            },
            // Process material
            [&](const MTLMatParams& m) -> bool {
                auto mat = comp::detail::createDirect<Material_WavefrontObj>(this, m);
                if (!mat) {
                    return false;
                }
                if (!mat->construct({})) {
                    return false;
                }
                assetsMap_[m.name] = int(assets_.size());
                assets_.push_back(std::move(mat));
                return true;
            },
            // Process texture
            [&](const MTLTextureParams& tex) -> bool {
                auto texture = comp::create<Texture>("texture::bitmap", this, {
                    {"path", tex.path}
                });
                if (!texture) {
                    return false;
                }
                assets_.push_back(std::move(texture));
                return true;
            });
    }
    
    virtual void createPrimitives(const CreatePrimitiveFunc& createPrimitive) const override {
        for (auto [mesh, material, light] : groups_) {
            auto* meshp = assets_.at(mesh).get();
            auto* materialp = assets_.at(material).get();
            createPrimitive(
                meshp,
                materialp,
                light < 0 ? nullptr : assets_.at(light).get());
        }
    }
};

LM_COMP_REG_IMPL(Model_WavefrontObj, "model::wavefrontobj");

LM_NAMESPACE_END(LM_NAMESPACE)
