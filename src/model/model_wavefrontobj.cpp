/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/model.h>
#include <lm/objloader.h>
#include <lm/mesh.h>
#include <lm/material.h>
#include <lm/texture.h>
#include <lm/logger.h>
#include <lm/film.h>
#include <lm/light.h>
#include <lm/json.h>
#include <lm/user.h>
#include <lm/serial.h>
#include <lm/surface.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

using namespace objloader;
class Mesh_WavefrontObj;

class Model_WavefrontObj final : public Model {
private:
    friend class Mesh_WavefrontObj;

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
        const std::string path = prop["path"];
        return objloader::load(path, geo_,
            // Process mesh
            [&](const OBJMeshFace& fs, const MTLMatParams& m) -> bool {
                // Create mesh
                const std::string meshName = fmt::format("mesh_{}", assets_.size());
                auto mesh = comp::create<Mesh>(
                    "mesh::wavefrontobj", makeLoc(meshName),
                    json::merge(prop, {
                        {"model_", this},
                        {"fs_", (const OBJMeshFace*)&fs}
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
                    const auto lightImplName = json::value<std::string>(prop, "light", "light::area");
                    const auto lightName = meshName + "_light";
                    auto light = comp::create<Light>(lightImplName, makeLoc(lightName), {
                        {"Ke", m.Ke},
                        {"mesh", makeLoc(meshName)}
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
                // User-specified material
                if (const auto it = prop.find("base_material"); it != prop.end()) {
                    auto mat = comp::create<Material>("material::proxy", makeLoc(m.name), {
                        {"ref", *it}
                    });
                    if (!mat) {
                        return false;
                    }
                    assetsMap_[m.name] = int(assets_.size());
                    assets_.push_back(std::move(mat));
                    return true;
                }

                // Load texture
                std::string mapKd_loc;
                if (!m.mapKd.empty()) {
                    // Use texture_<filename> as an identifier
                    const auto id = "texture_" + std::filesystem::path(m.mapKd).stem().string();

                    // Check if already loaded
                    if (auto it = assetsMap_.find(id); it == assetsMap_.end()) {
                        // If not loaded, load the texture
                        const auto textureAssetName = json::value<std::string>(prop, "texture", "texture::bitmap");
                        auto texture = comp::create<Texture>(textureAssetName, makeLoc(id), {
                            {"path", (std::filesystem::path(path).remove_filename()/m.mapKd).string()}
                        });
                        if (!texture) {
                            return false;
                        }
                        assetsMap_[id] = int(assets_.size());
                        assets_.push_back(std::move(texture));
                    }

                    // Locator of the texture
                    mapKd_loc = makeLoc(id);
                }

                // Default mixture material
                auto mat = comp::create<Material>(
                    "material::wavefrontobj", makeLoc(m.name),
                    json::merge(prop, {
                        {"matparams_", (const MTLMatParams*)&m},
                        {"mapKd_", mapKd_loc}
                    }
                ));
                if (!mat) {
                    return false;
                }
                assetsMap_[m.name] = int(assets_.size());
                assets_.push_back(std::move(mat));

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

class Mesh_WavefrontObj final  : public Mesh {
private:
    Model_WavefrontObj* model_;
    OBJMeshFace fs_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(model_, fs_);
    }

    virtual void foreachUnderlying(const ComponentVisitor& visit) override {
        comp::visit(visit, model_);
    }

public:
    virtual bool construct(const Json& prop) override {
        model_ = prop["model_"].get<Model_WavefrontObj*>();
        fs_ = *prop["fs_"].get<const OBJMeshFace*>();
        return true;
    }

    virtual void foreachTriangle(const ProcessTriangleFunc& processTriangle) const override {
        for (int fi = 0; fi < int(fs_.size()); fi += 3) {
            processTriangle(fi, triangleAt(fi/3));
        }
    }

    virtual Tri triangleAt(int face) const override {
        const auto& geo_ = model_->geo_;
        const auto f1 = fs_[3*face];
        const auto f2 = fs_[3*face+1];
        const auto f3 = fs_[3*face+2];
        return {
            { geo_.ps[f1.p], f1.n<0 ? Vec3() : geo_.ns[f1.n], f1.t<0 ? Vec2() : geo_.ts[f1.t] },
            { geo_.ps[f2.p], f2.n<0 ? Vec3() : geo_.ns[f2.n], f2.t<0 ? Vec2() : geo_.ts[f2.t] },
            { geo_.ps[f3.p], f3.n<0 ? Vec3() : geo_.ns[f3.n], f3.t<0 ? Vec2() : geo_.ts[f3.t] }
        };
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

class Material_WavefrontObj final : public Material {
private:
    // Material parameters of MLT file
    MTLMatParams objmat_;

    // Underlying material components
    std::vector<Component::Ptr<Material>> materials_;
    std::unordered_map<std::string, int> typeToIndexMap_;

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

    virtual Component* underlying(const std::string& name) const override {
        return materials_.at(typeToIndexMap_.at(name)).get();
    }

    virtual void foreachUnderlying(const ComponentVisitor& visit) override {
        comp::visit(visit, maskTex_);
        for (auto& material : materials_) {
            comp::visit(visit, material);
        }
    }

    virtual Json underlyingValue(const std::string& query) const override {
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
    int addMaterial(const std::string& name, const std::string& type, const Json& prop) {
        auto p = comp::create<Material>(name, makeLoc(type), prop);
        if (!p) {
            return -1;
        }
        int index = int(materials_.size());
        materials_.push_back(std::move(p));
        typeToIndexMap_[type] = index;
        return index;
    }

public:
    virtual bool construct(const Json& prop) override {
        objmat_ = *prop["matparams_"].get<const MTLMatParams*>();
        objmat_.mapKd = prop["mapKd_"];

        // Make parent component as a parent for the newly created components
        if (objmat_.illum == 7) {
            // Glass material
            const auto glassMaterialName = json::value<std::string>(prop, "glass", "material::glass");
            glass_ = addMaterial(glassMaterialName, "glass", {
                {"Ni", objmat_.Ni}
            });
            if (glass_ < 0) {
                return false;
            }
        }
        else if (objmat_.illum == 5) {
            // Mirror material
            const auto mirrorMaterialName = json::value<std::string>(prop, "mirror", "material::mirror");
            mirror_ = addMaterial(mirrorMaterialName, "mirror", {});
            if (mirror_ < 0) {
                return false;
            }
        }
        else {
            // Diffuse material
            const auto diffuseMaterialName = json::value<std::string>(prop, "diffuse", "material::diffuse");
            Json matprop{ {"Kd", objmat_.Kd} };
            if (!objmat_.mapKd.empty()) {
                matprop["mapKd"] = objmat_.mapKd;
            }
            diffuse_ = addMaterial(diffuseMaterialName, "diffuse", matprop);
            if (diffuse_ < 0) {
                return false;
            }

            // Glossy material
            const auto glossyMaterialName = json::value<std::string>(prop, "glossy", "material::glossy");
            const auto r = 2_f / (2_f + objmat_.Ns);
            const auto as = math::safeSqrt(1_f - objmat_.an * .9_f);
            glossy_ = addMaterial(glossyMaterialName, "glossy", {
                {"Ks", objmat_.Ks},
                {"ax", std::max(1e-3_f, r / as)},
                {"ay", std::max(1e-3_f, r * as)}
            });
            if (glossy_ < 0) {
                return false;
            }

            // Mask texture
            if (!objmat_.mapKd.empty()) {
                auto* texture = lm::comp::get<Texture>(objmat_.mapKd);
                if (texture->hasAlpha()) {
                    maskTex_ = texture;
                    mask_ = addMaterial("material::mask", "mask", {});
                    if (mask_ < 0) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    virtual bool isSpecular(const PointGeometry& geom, int comp) const override {
        return materials_.at(comp)->isSpecular(geom, SurfaceComp::DontCare);
    }

    virtual std::optional<MaterialDirectionSample> sample(Rng& rng, const PointGeometry& geom, Vec3 wi) const override {
        // Select component
        const auto [comp, weight] = [&]() -> std::tuple<int, Float> {
            // Glass
            if (glass_ >= 0) {
                return { glass_, 1_f };
            }

            // Mirror
            if (mirror_ >= 0) {
                return { mirror_, 1_f };
            }

            // Diffuse or glossy or mask
            const auto* D = materials_.at(diffuse_).get();
            const auto* G = materials_.at(glossy_).get();
            const auto wd = [&]() {
                const auto wd = glm::compMax(*D->reflectance(geom, SurfaceComp::DontCare));
                const auto ws = glm::compMax(*G->reflectance(geom, SurfaceComp::DontCare));
                if (wd == 0_f && ws == 0_f) {
                    return 1_f;
                }
                return wd / (wd + ws);
            }();
            if (rng.u() < wd) {
                if (maskTex_ && rng.u() > maskTex_->evalAlpha(geom.t)) {
                    return { mask_, 1_f/wd };
                }
                else {
                    return { diffuse_, 1_f/wd };
                }
            }
            return { glossy_, 1_f/(1_f-wd) };
        }();

        // Sample a ray
        auto s = materials_.at(comp)->sample(rng, geom, wi);
        if (!s) {
            return {};
        }
        return MaterialDirectionSample{
            s->wo,
            comp,
            s->weight * weight
        };
    }

    virtual std::optional<Vec3> reflectance(const PointGeometry& geom, int comp) const override {
        if (comp >= 0) {
            return materials_.at(comp)->reflectance(geom, SurfaceComp::DontCare);
        }
        if (diffuse_ >= 0) {
            return materials_.at(diffuse_)->reflectance(geom, SurfaceComp::DontCare);
        }
        return {};
    }

    virtual Float pdf(const PointGeometry& geom, int comp, Vec3 wi, Vec3 wo) const override {
        return materials_.at(comp)->pdf(geom, SurfaceComp::DontCare, wi, wo);
    }

    virtual Vec3 eval(const PointGeometry& geom, int comp, Vec3 wi, Vec3 wo) const override {
        return materials_.at(comp)->eval(geom, SurfaceComp::DontCare, wi, wo);
    }
};

LM_COMP_REG_IMPL(Material_WavefrontObj, "material::wavefrontobj");

LM_NAMESPACE_END(LM_NAMESPACE)
