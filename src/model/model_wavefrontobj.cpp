/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
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
#include <lm/user.h>
#include <lm/serial.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// ----------------------------------------------------------------------------

// Surface geometry shared among meshes
struct OBJSurfaceGeometry {
    std::vector<Vec3> ps;   // Positions
    std::vector<Vec3> ns;   // Normals
    std::vector<Vec2> ts;   // Texture coordinates

    template <typename Archive>
    void serialize(Archive& ar) {
        ar(ps, ns, ts);
    }
};

// Face indices
struct OBJMeshFaceIndex {
    int p = -1;         // Index of position
    int t = -1;         // Index of texture coordinates
    int n = -1;         // Index of normal

    template <typename Archive>
    void serialize(Archive& ar) {
        ar(p, t, n);
    }
};

// Face
using OBJMeshFace = std::vector<OBJMeshFaceIndex>;

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
    std::string mapKd;  // Name of texture for Kd
    Float Ni;           // Index of refraction
    Float Ns;           // Specular exponent for phong shading
    Float an;           // Anisotropy

    template <typename Archive>
    void serialize(Archive& ar) {
        ar(name, illum, Kd, Ks, Ke, mapKd, Ni, Ns, an);
    }
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
        std::optional<int>(const OBJMeshFace& fs, const MTLMatParams& m)>;
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
    void nextString(char *&t, char (&name)[N]) {
        #if LM_COMPILER_MSVC
        sscanf_s(t, "%s", name, N);
        #else
        sscanf(t, "%s", name);
        #endif
    };

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
                // Use filename as an identifi]er
                const auto id = std::filesystem::path(name).stem().string();
                m.mapKd = id;
                // Check if the texture is alread loaded
                auto it = tsmap_.find(id);
                if (it == tsmap_.end()) {
                    // Register a new texture
                    ts_.push_back({id, (std::filesystem::path(p).remove_filename()/name).string()});
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

class Mesh_WavefrontObj;
class Material_WavefrontObj;

class Model_WavefrontObj : public Model {
private:
    friend class Mesh_WavefrontObj;
    friend class Material_WavefrontObj;

private:
    // Surface geometry
    OBJSurfaceGeometry geo_;

    // Underlying assets
    std::vector<Component::Ptr<Component>> assets_;
    std::unordered_map<std::string, int> assetsMap_;

    // Mesh group which assocites a mesh and a material
    struct Group {
        int mesh;
        int material;
        int light;

        template <typename Archive>
        void serialize(Archive& ar) {
            ar(mesh, material, light);
        }
    };
    std::vector<Group> groups_;

    // Temporary parameters accessed from underlying assets
    // Origial material parameters
    MTLMatParams currentMatParams_;
    // Face indices
    OBJMeshFace currentFs_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(geo_, groups_, assetsMap_, assets_);
    }

    virtual void foreachUnderlying(const ComponentVisitor& visit) override {
        for (auto& asset : assets_) {
            comp::visit(visit, asset);
        }
    }

public:
    virtual Component* underlying(const std::string& name) const override {
        return assets_[assetsMap_.at(name)].get();
    }

    virtual bool construct(const Json& prop) override {
        WavefrontOBJParser parser;
        return parser.parse(prop["path"], geo_,
            // Process mesh}
            [&](const OBJMeshFace& fs, const MTLMatParams& m) -> std::optional<int> {
                currentFs_ = fs;

                // Create mesh
                const std::string meshName = fmt::format("mesh{}", assets_.size());
                auto mesh = comp::create<Mesh>(
                    "mesh::wavefrontobj", makeLoc(loc(), meshName),
                    json::merge(prop, {
                        {"model_", loc()}
                    }
                ));
                if (!mesh) {
                    return false;
                }
                assetsMap_[meshName] = int(assets_.size());
                assets_.push_back(std::move(mesh));

                // Create area light if Ke > 0
                int lightIndex = -1;
                if (glm::compMax(m.Ke) > 0_f) {
                    const auto lightName = meshName + "_light";
                    auto light = comp::create<Light>("light::area", makeLoc(loc(), lightName), {
                        {"Ke", m.Ke},
                        {"mesh", "global:" + makeLoc(loc(), meshName)}
                    });
                    if (!light) {
                        return false;
                    }
                    lightIndex = int(assets_.size());
                    assetsMap_[lightName] = int(assets_.size());
                    assets_.push_back(std::move(light));
                }

                // Create mesh group
                groups_.push_back({ assetsMap_[meshName], assetsMap_[m.name], lightIndex });

                return true;
            },
            // Process material
            [&](const MTLMatParams& m) -> bool {
                if (const auto it = prop.find("base_material"); it != prop.end()) {
                    // Use user-specified material
                    auto mat = comp::create<Material>("material::proxy", makeLoc(loc(), m.name), {
                        {"ref", *it}
                    });
                    if (!mat) {
                        return false;
                    }
                    assetsMap_[m.name] = int(assets_.size());
                    assets_.push_back(std::move(mat));
                }
                else {
                    // Default mixture material
                    currentMatParams_ = m;
                    auto mat = comp::create<Material>(
                        "material::wavefrontobj", makeLoc(loc(), m.name),
                        json::merge(prop, {
                            {"model_", loc()}
                        }
                    ));
                    if (!mat) {
                        return false;
                    }
                    assetsMap_[m.name] = int(assets_.size());
                    assets_.push_back(std::move(mat));
                }
                return true;
            },
            // Process texture
            [&](const MTLTextureParams& tex) -> bool {
                const auto textureAssetName = json::valueOr<std::string>(prop, "texture", "texture::bitmap");
                auto texture = comp::create<Texture>(textureAssetName, makeLoc(loc(), tex.name), {
                    {"path", tex.path}
                });
                if (!texture) {
                    return false;
                }
                assetsMap_[tex.name] = int(assets_.size());
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

// ----------------------------------------------------------------------------

class Mesh_WavefrontObj : public Mesh {
private:
    Model_WavefrontObj* model_;
    std::vector<OBJMeshFaceIndex> fs_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(model_, fs_);
    }

    virtual void foreachUnderlying(const ComponentVisitor& visit) override {
        comp::visit(visit, model_);
    }

public:
    virtual bool construct(const Json& prop) override {
        model_ = comp::get<Model_WavefrontObj>(prop["model_"]);
        fs_ = model_->currentFs_;
        return true;
    }

    virtual void foreachTriangle(const ProcessTriangleFunc& processTriangle) const override {
        const auto& geo_ = model_->geo_;
        for (int fi = 0; fi < int(fs_.size()); fi += 3) {
            const auto p1 = geo_.ps[fs_[fi].p];
            const auto p2 = geo_.ps[fs_[fi + 1].p];
            const auto p3 = geo_.ps[fs_[fi + 2].p];
            processTriangle(fi, p1, p2, p3);
        }
    }

    virtual Tri triangleAt(int face) const override {
        const auto& geo_ = model_->geo_;
        const auto p1 = geo_.ps[fs_[3*face].p];
        const auto p2 = geo_.ps[fs_[3*face + 1].p];
        const auto p3 = geo_.ps[fs_[3*face + 2].p];
        return { p1, p2, p3 };
    }

    virtual Point surfacePoint(int face, Vec2 uv) const override {
        const auto& geo_ = model_->geo_;
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

LM_COMP_REG_IMPL(Mesh_WavefrontObj, "mesh::wavefrontobj");

// ----------------------------------------------------------------------------

class Material_WavefrontObj : public Material {
private:
    // Model asset
    Model_WavefrontObj* model_;

    // Material parameters of MLT file
    MTLMatParams objmat_;

    // Underlying material components
    std::vector<Component::Ptr<Material>> materials_;

    // Component indices
    int diffuse_ = -1;
    int glossy_ = -1;
    int glass_ = -1;
    int mirror_ = -1;
    int mask_ = -1;

    // Texture for the alpha mask
    Texture* maskTex_ = nullptr;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(objmat_, materials_,
           diffuse_, glossy_, glass_, mirror_, mask_,
           maskTex_);
    }

    virtual void foreachUnderlying(const ComponentVisitor& visit) override {
        comp::visit(visit, model_);
        comp::visit(visit, maskTex_);
        for (auto& material : materials_) {
            comp::visit(visit, material);
        }
    }

    virtual Json underlyingValue(const std::string& query) const {
        LM_UNUSED(query);
        return {
            { "name", objmat_.name },
            { "illum", objmat_.illum },
            { "Kd", objmat_.Kd },
            { "Ks", objmat_.Ks },
            { "Ke", objmat_.Ke },
            { "mapKd", objmat_.mapKd },
            { "Ni", objmat_.Ni },
            { "Ns", objmat_.Ns },
            { "an", objmat_.an },
        };
    }

private:
    int addMaterial(const std::string& name, const Json& prop) {
        auto p = comp::create<Material>(name, "", prop);
        if (!p) {
            return -1;
        }
        int index = int(materials_.size());
        materials_.push_back(std::move(p));
        return index;
    }

public:
    virtual bool construct(const Json& prop) override {
        // Model asset
        model_ = comp::get<Model_WavefrontObj>(prop["model_"]);
        objmat_ = model_->currentMatParams_;

        // Make parent component as a parent for the newly created components
        if (objmat_.illum == 7) {
            // Glass material
            const auto glassMaterialName = json::valueOr<std::string>(prop, "glass", "material::glass");
            glass_ = addMaterial(glassMaterialName, {
                {"Ni", objmat_.Ni}
            });
            if (glass_ < 0) {
                return false;
            }
        }
        else if (objmat_.illum == 5) {
            // Mirror material
            const auto mirrorMaterialName = json::valueOr<std::string>(prop, "mirror", "material::mirror");
            mirror_ = addMaterial(mirrorMaterialName, {});
            if (mirror_ < 0) {
                return false;
            }
        }
        else {
            // Diffuse material
            const auto diffuseMaterialName = json::valueOr<std::string>(prop, "diffuse", "material::diffuse");
            diffuse_ = addMaterial(diffuseMaterialName, {
                {"Kd", objmat_.Kd},
                {"mapKd", objmat_.mapKd.empty()
                    ? "" : "global:" + makeLoc(parentLoc(), objmat_.mapKd)}
            });
            if (diffuse_ < 0) {
                return false;
            }

            // Glossy material
            const auto glossyMaterialName = json::valueOr<std::string>(prop, "glossy", "material::glossy");
            const auto r = 2_f / (2_f + objmat_.Ns);
            const auto as = math::safeSqrt(1_f - objmat_.an * .9_f);
            glossy_ = addMaterial(glossyMaterialName, {
                {"Ks", objmat_.Ks},
                {"ax", std::max(1e-3_f, r / as)},
                {"ay", std::max(1e-3_f, r * as)}
            });
            if (glossy_ < 0) {
                return false;
            }

            // Mask texture
            if (!objmat_.mapKd.empty()) {
                auto* texture = lm::comp::get<Texture>(makeLoc(parentLoc(), objmat_.mapKd));
                if (texture->hasAlpha()) {
                    maskTex_ = texture;
                    mask_ = addMaterial("material::mask", {});
                    if (mask_ < 0) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    virtual bool isSpecular(const SurfacePoint& sp) const override {
        return materials_.at(sp.comp)->isSpecular(sp);
    }

    virtual std::optional<RaySample> sampleRay(Rng& rng, const SurfacePoint& sp, Vec3 wi) const {
        // Select component
        const auto [compIndex, weight] = [&]() -> std::tuple<int, Float> {
            // Glass or mirror
            if (glass_ >= 0 || mirror_ >= 0) {
                return { mirror_ < 0 ? glass_ : mirror_, 1_f };
            }

            // Diffuse or glossy or mask
            const auto* D = materials_.at(diffuse_).get();
            const auto* G = materials_.at(glossy_).get();
            const auto wd = [&]() {
                const auto wd = glm::compMax(*D->reflectance(sp));
                const auto ws = glm::compMax(*G->reflectance(sp));
                return wd == 0_f && ws == 0_f ? 1_f : wd / (wd + ws);
            }();
            if (rng.u() < wd) {
                if (maskTex_ && rng.u() > maskTex_->evalAlpha(sp.t)) {
                    return { mask_, 1_f/wd };
                }
                else {
                    return { diffuse_, 1_f / wd };
                }
            }
            return { glossy_, 1_f/(1_f-wd) };
        }();

        // Sample a ray
        auto s = materials_.at(compIndex)->sampleRay(rng, sp, wi);
        if (!s) {
            return {};
        }
        return s->asComp(compIndex).multWeight(weight);
    }

    virtual std::optional<Vec3> reflectance(const SurfacePoint& sp) const {
        if (sp.comp >= 0) {
            return materials_.at(sp.comp)->reflectance(sp);
        }
        if (diffuse_ >= 0) {
            return materials_.at(diffuse_)->reflectance(sp);
        }
        return {};
    }

    virtual Float pdf(const SurfacePoint& sp, Vec3 wi, Vec3 wo) const {
        return materials_.at(sp.comp)->pdf(sp, wi, wo);
    }

    virtual Vec3 eval(const SurfacePoint& sp, Vec3 wi, Vec3 wo) const {
        return materials_.at(sp.comp)->eval(sp, wi, wo);
    }
};

LM_COMP_REG_IMPL(Material_WavefrontObj, "material::wavefrontobj");

LM_NAMESPACE_END(LM_NAMESPACE)
