/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/model.h>
#include <lm/objloader.h>
#include <lm/mesh.h>
#include <lm/material.h>
#include <lm/texture.h>
#include <lm/film.h>
#include <lm/light.h>
#include <lm/surface.h>

#define MODEL_WAVEFRONTOBJ_NEW_MIXTURE_MATERIAL 1

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
    std::unordered_map<std::string, int> assets_map_;

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
        ar(geo_, groups_, assets_map_, assets_);
    }

    virtual void foreach_underlying(const ComponentVisitor& visit) override {
        for (auto& asset : assets_) {
            comp::visit(visit, asset);
        }
    }

public:
    virtual Component* underlying(const std::string& name) const override {
        return assets_[assets_map_.at(name)].get();
    }

	virtual void construct(const Json& prop) override {
        const std::string path = json::value<std::string>(prop, "path");
        bool result = objloader::load(path, geo_,
            // Process mesh
            [&](const OBJMeshFace& fs, const MTLMatParams& m) -> bool {
                // Create mesh
                const std::string mesh_name = fmt::format("mesh_{}", assets_.size());
                auto mesh = comp::create<Mesh>(
                    "mesh::wavefrontobj", make_loc(mesh_name),
                    json::merge(prop, {
                        {"model_", this},
                        {"fs_", (const OBJMeshFace*)&fs}
                    }
                ));
                if (!mesh) {
                    return false;
                }
                assets_map_[mesh_name] = int(assets_.size());
                assets_.push_back(std::move(mesh));

                // --------------------------------------------------------------------------------

                // Create area light if Ke > 0
                int light_index = -1;
                if (glm::compMax(m.Ke) > 0_f) {
                    const auto light_impl_name = json::value<std::string>(prop, "light", "light::area");
                    const auto light_name = mesh_name + "_light";
                    auto light = comp::create<Light>(light_impl_name, make_loc(light_name), {
                        {"Ke", m.Ke},
                        {"mesh", make_loc(mesh_name)}
                    });
                    if (!light) {
                        return false;
                    }
                    light_index = int(assets_.size());
                    assets_map_[light_name] = int(assets_.size());
                    assets_.push_back(std::move(light));
                }

                // --------------------------------------------------------------------------------

                // Create mesh group
                groups_.push_back({ assets_map_[mesh_name], assets_map_[m.name], light_index });

                return true;
            },

            // ------------------------------------------------------------------------------------

            // Process material
            [&](const MTLMatParams& m) -> bool {
                // Load user-specified material if given
                if (const auto it = prop.find("base_material"); it != prop.end()) {
                    auto mat = comp::create<Material>("material::proxy", make_loc(m.name), {
                        {"ref", *it}
                    });
                    if (!mat) {
                        return false;
                    }
                    assets_map_[m.name] = int(assets_.size());
                    assets_.push_back(std::move(mat));
                    return true;
                }

                // --------------------------------------------------------------------------------

                // Load texture
                std::string mapKd_loc;
                if (!m.mapKd.empty()) {
                    // Use texture_<filename> as an identifier
                    const auto id = "texture_" + fs::path(m.mapKd).stem().string();

                    // Check if already loaded
                    if (auto it = assets_map_.find(id); it == assets_map_.end()) {
                        // If not loaded, load the texture
                        const auto textureAssetName = json::value<std::string>(prop, "texture", "texture::bitmap");
                        auto texture = comp::create<Texture>(textureAssetName, make_loc(id), {
                            {"path", (fs::path(path).remove_filename()/m.mapKd).string()}
                        });
                        if (!texture) {
                            return false;
                        }
                        assets_map_[id] = int(assets_.size());
                        assets_.push_back(std::move(texture));
                    }

                    // Locator of the texture
                    mapKd_loc = make_loc(id);
                }

                // --------------------------------------------------------------------------------

#if MODEL_WAVEFRONTOBJ_NEW_MIXTURE_MATERIAL
                // Load material
                Ptr<Material> mat;
                if (m.illum == 7) {
                    // Glass
                    mat = comp::create<Material>(
                        "material::glass", make_loc(m.name), {{"Ni", m.Ni}});
                }
                else if (m.illum == 5) {
                    // Mirror
                    mat = comp::create<Material>(
                        "material::mirror", make_loc(m.name), {});
                }
                else {
                    // Convert parameter for anisotropic GGX 
                    const auto r = 2_f / (2_f + m.Ns);
                    const auto as = math::safe_sqrt(1_f - m.an * .9_f);

                    // Default mixture material of D and G
                    mat = comp::create<Material>(
                        "material::wavefrontobj_mixture", make_loc(m.name), {
                            {"Kd", m.Kd},
                            {"mapKd", mapKd_loc},
                            {"Ks", m.Ks},
                            {"ax", std::max(1e-3_f, r / as)},
                            {"ay", std::max(1e-3_f, r * as)}
                        });
                }
                if (!mat) {
                    return false;
                }
                assets_map_[m.name] = int(assets_.size());
                assets_.push_back(std::move(mat));
#else


                // Load material
                auto mat = comp::create<Material>(
                    "material::wavefrontobj", make_loc(m.name),
                    json::merge(prop, {
                        {"matparams_", (const MTLMatParams*)&m},
                        {"mapKd_", mapKd_loc}
                    }
                ));
                if (!mat) {
                    return false;
                }
                assets_map_[m.name] = int(assets_.size());
                assets_.push_back(std::move(mat));
#endif

                return true;
            });
        if (!result) {
            LM_THROW_EXCEPTION_DEFAULT(Error::IOError);
        }
    }
    
    virtual void create_primitives(const CreatePrimitiveFunc& createPrimitive) const override {
        for (auto [mesh, material, light] : groups_) {
            auto* meshp = assets_.at(mesh).get();
            auto* materialp = assets_.at(material).get();
            createPrimitive(
                meshp,
                materialp,
                light < 0 ? nullptr : assets_.at(light).get());
        }
    }

	virtual void foreach_node(const VisitNodeFuncType&) const override {
        LM_THROW_EXCEPTION_DEFAULT(Error::Unsupported);
	}
};

LM_COMP_REG_IMPL(Model_WavefrontObj, "model::wavefrontobj");

// ------------------------------------------------------------------------------------------------

class Mesh_WavefrontObj final  : public Mesh {
private:
    Model_WavefrontObj* model_;
    OBJMeshFace fs_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(model_, fs_);
    }

    virtual void foreach_underlying(const ComponentVisitor& visit) override {
        comp::visit(visit, model_);
    }

public:
    virtual void construct(const Json& prop) override {
        model_ = prop["model_"].get<Model_WavefrontObj*>();
        fs_ = *prop["fs_"].get<const OBJMeshFace*>();
    }

    virtual void foreach_triangle(const ProcessTriangleFunc& processTriangle) const override {
        for (int fi = 0; fi < int(fs_.size())/3; fi++) {
            processTriangle(fi, triangle_at(fi));
        }
    }

    virtual Tri triangle_at(int face) const override {
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

    virtual Point surface_point(int face, Vec2 uv) const override {
        const auto& geo_ = model_->geo_;
        const auto i1 = fs_[3*face];
        const auto i2 = fs_[3*face+1];
        const auto i3 = fs_[3*face+2];
        const auto p1 = geo_.ps[i1.p];
        const auto p2 = geo_.ps[i2.p];
        const auto p3 = geo_.ps[i3.p];
        return {
            // Position
            math::mix_barycentric(p1, p2, p3, uv),
            // Normal. Use geometry normal if the attribute is missing.
            i1.n < 0 ? glm::normalize(glm::cross(p2-p1, p3-p1))
                     : glm::normalize(math::mix_barycentric(
                         geo_.ns[i1.n], geo_.ns[i2.n], geo_.ns[i3.n], uv)),
            // Texture coordinates
            i1.t < 0 ? Vec2(0) : math::mix_barycentric(
                geo_.ts[i1.t], geo_.ts[i2.t], geo_.ts[i3.t], uv)
        };
    }

    virtual int num_triangles() const override {
        return int(fs_.size()) / 3;
    }
};

LM_COMP_REG_IMPL(Mesh_WavefrontObj, "mesh::wavefrontobj");

// ------------------------------------------------------------------------------------------------

class Material_WavefrontObj_Mixture final : public Material {
private:
    Component::Ptr<Material> diffuse_;
    Component::Ptr<Material> glossy_;
    Component::Ptr<Material> alpha_mask_;
    Texture* mask_tex_ = nullptr;

    // Component indices
    enum {
        Comp_Diffuse = 0,   // Diffuse material
        Comp_Glossy  = 1,   // Glossy material
        Comp_Alpha   = 2,   // Alpha mask
    };

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(diffuse_, glossy_, alpha_mask_, mask_tex_);
    }

    virtual Component* underlying(const std::string& name) const override {
        if (name == "diffuse") return diffuse_.get();
        else if (name == "glossy") return glossy_.get();
        else if (name == "alpha_mask") return alpha_mask_.get();
        return nullptr;
    }

    virtual void foreach_underlying(const ComponentVisitor& visit) override {
        comp::visit(visit, diffuse_);
        comp::visit(visit, glossy_);
        comp::visit(visit, alpha_mask_);
        comp::visit(visit, mask_tex_);
    }

private:
    // Get material by component index
    Material* material_by_comp(int comp) const {
        switch (comp) {
            case Comp_Diffuse:
                return diffuse_.get();
            case Comp_Glossy:
                return glossy_.get();
            case Comp_Alpha:
                return alpha_mask_.get();
        }
        return nullptr;
    }

    // Compute selection weight
    Float diffuse_selection_weight(const PointGeometry& geom) const {
        const auto weight_d = [&]() {
            const auto weight_d = glm::compMax(*diffuse_->reflectance(geom, SurfaceComp::DontCare));
            const auto weight_g = glm::compMax(*glossy_->reflectance(geom, SurfaceComp::DontCare));
            if (weight_d == 0_f && weight_g == 0_f) {
                return 1_f;
            }
            return weight_d / (weight_d + weight_g);
        }();
        return weight_d;
    }

    // Evaluate alpha value
    Float eval_alpha(const PointGeometry& geom) const {
        return !mask_tex_ ? 1_f : mask_tex_->eval_alpha(geom.t);
    }

    // Component selection
    int sample_comp_select(Rng& rng, const PointGeometry& geom) const {
        // Alpha mask
        const auto alpha = eval_alpha(geom);
        if (rng.u() > alpha) {
            return Comp_Alpha;
        }

        // Difuse
        const auto weight_d = diffuse_selection_weight(geom);
        if (rng.u() < weight_d) {
            return Comp_Diffuse;
        }

        // Glossy
        return Comp_Glossy;
    }

    // Component selection PMF
    Float pdf_comp_select(const PointGeometry& geom, int comp) const {
        // Alpha mask
        const auto alpha = eval_alpha(geom);
        if (comp == Comp_Alpha) {
            return 1_f - alpha;
        }

        // Diffuse
        const auto weight_d = diffuse_selection_weight(geom);
        if (comp == Comp_Diffuse) {
            return alpha * weight_d;
        }
        
        // Glossy
        assert(comp == Comp_Glossy);
        return alpha * (1_f - weight_d);
    }

    // Evaluate mixture weight
    Float eval_mix_weight(const PointGeometry& geom, int comp) const {
        const auto alpha = eval_alpha(geom);
        return comp == Comp_Alpha ? (1_f - alpha) : alpha;
    }

public:
    virtual void construct(const Json& prop) override {
        const auto Kd = json::value<Vec3>(prop, "Kd");
        const auto mapKd = json::value<std::string>(prop, "mapKd");
        const auto Ks = json::value<Vec3>(prop, "Ks");
        const auto ax = json::value<Float>(prop, "ax");
        const auto ay = json::value<Float>(prop, "ay");

        // Diffuse material
        diffuse_ = comp::create<Material>(
            "material::diffuse", make_loc("diffuse"), {
                {"Kd", Kd},
                {"mapKd", mapKd}
            });

        // Glossy material
        glossy_ = comp::create<Material>(
            "material::glossy", make_loc("glossy"), {
                {"Ks", Ks},
                {"ax", ax},
                {"ay", ay}
            });

        // Alpha mask
        alpha_mask_ = comp::create<Material>(
            "material::mask", make_loc("alpha_mask"), {});
        if (!mapKd.empty()) {
            auto* texture = comp::get<Texture>(mapKd);
            if (texture && texture->has_alpha()) {
                mask_tex_ = texture;
            }
        }
    }

    virtual bool is_specular(const PointGeometry& geom, int comp) const override {
        return material_by_comp(comp)->is_specular(geom, SurfaceComp::DontCare);
    }

    virtual std::optional<MaterialDirectionSample> sample(Rng& rng, const PointGeometry& geom, Vec3 wi) const override {
        const int comp = sample_comp_select(rng, geom);
        const auto pdf_sel = pdf_comp_select(geom, comp);
        const auto w_mix = eval_mix_weight(geom, comp);
        const auto s = material_by_comp(comp)->sample(rng, geom, wi);
        if (!s) {
            return {};
        }
        return MaterialDirectionSample{
            s->wo,
            comp,
            s->weight * w_mix / pdf_sel
        };
    }

    virtual std::optional<Vec3> reflectance(const PointGeometry& geom, int) const override {
        // Always use diffuse
        return diffuse_->reflectance(geom, SurfaceComp::DontCare);
    }

    virtual Float pdf(const PointGeometry& geom, int comp, Vec3 wi, Vec3 wo) const override {
        return material_by_comp(comp)->pdf(geom, SurfaceComp::DontCare, wi, wo);
    }

    virtual Float pdf_comp(const PointGeometry& geom, int comp, Vec3) const override {
        return pdf_comp_select(geom, comp);
    }

    virtual Vec3 eval(const PointGeometry& geom, int comp, Vec3 wi, Vec3 wo) const override {
        const auto w_mix = eval_mix_weight(geom, comp);
        return w_mix * material_by_comp(comp)->eval(geom, SurfaceComp::DontCare, wi, wo);
    }
};

LM_COMP_REG_IMPL(Material_WavefrontObj_Mixture, "material::wavefrontobj_mixture");

// ------------------------------------------------------------------------------------------------

class Material_WavefrontObj final : public Material {
private:
    // Material parameters of MLT file
    MTLMatParams objmat_;

    // Underlying material components
    std::vector<Component::Ptr<Material>> materials_;
    std::unordered_map<std::string, int> type_to_index_map_;

    // Component indices
    int diffuse_ = -1;
    int glossy_ = -1;
    int glass_ = -1;
    int mirror_ = -1;
    int mask_ = -1;

    // Texture for the alpha mask
    Texture* mask_tex_ = nullptr;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(objmat_, materials_, type_to_index_map_,
           diffuse_, glossy_, glass_, mirror_, mask_,
           mask_tex_);
    }

    virtual Component* underlying(const std::string& name) const override {
        if (auto it = type_to_index_map_.find(name); it != type_to_index_map_.end()) {
            return materials_.at(it->second).get();
        }
        return nullptr;
    }

    virtual void foreach_underlying(const ComponentVisitor& visit) override {
        comp::visit(visit, mask_tex_);
        for (auto& material : materials_) {
            comp::visit(visit, material);
        }
    }

    virtual Json underlying_value(const std::string& query) const override {
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
    int add_material(const std::string& name, const std::string& type, const Json& prop) {
        auto p = comp::create<Material>(name, make_loc(type), prop);
        if (!p) {
            return -1;
        }
        int index = int(materials_.size());
        materials_.push_back(std::move(p));
        type_to_index_map_[type] = index;
        return index;
    }

public:
    virtual void construct(const Json& prop) override {
        objmat_ = *prop["matparams_"].get<const MTLMatParams*>();
        objmat_.mapKd = prop["mapKd_"];

        // Make parent component as a parent for the newly created components
        if (objmat_.illum == 7) {
            // Glass material
            const auto glass_material_name = json::value<std::string>(prop, "glass", "material::glass");
            glass_ = add_material(glass_material_name, "glass", {
                {"Ni", objmat_.Ni}
            });
            if (glass_ < 0) {
                LM_THROW_EXCEPTION_DEFAULT(Error::InvalidArgument);
            }
        }
        else if (objmat_.illum == 5) {
            // Mirror material
            const auto mirror_material_name = json::value<std::string>(prop, "mirror", "material::mirror");
            mirror_ = add_material(mirror_material_name, "mirror", {});
            if (mirror_ < 0) {
                LM_THROW_EXCEPTION_DEFAULT(Error::InvalidArgument);
            }
        }
        else {
            // Diffuse material
            const auto diffuse_material_name = json::value<std::string>(prop, "diffuse", "material::diffuse");
            Json matprop{ {"Kd", objmat_.Kd} };
            if (!objmat_.mapKd.empty()) {
                matprop["mapKd"] = objmat_.mapKd;
            }
            diffuse_ = add_material(diffuse_material_name, "diffuse", matprop);
            if (diffuse_ < 0) {
                LM_THROW_EXCEPTION_DEFAULT(Error::InvalidArgument);
            }

            // Glossy material
            const auto glossy_material_name = json::value<std::string>(prop, "glossy", "material::glossy");
            const auto r = 2_f / (2_f + objmat_.Ns);
            const auto as = math::safe_sqrt(1_f - objmat_.an * .9_f);
            glossy_ = add_material(glossy_material_name, "glossy", {
                {"Ks", objmat_.Ks},
                {"ax", std::max(1e-3_f, r / as)},
                {"ay", std::max(1e-3_f, r * as)}
            });
            if (glossy_ < 0) {
                LM_THROW_EXCEPTION_DEFAULT(Error::InvalidArgument);
            }

            // Mask texture
            if (!objmat_.mapKd.empty()) {
                auto* texture = lm::comp::get<Texture>(objmat_.mapKd);
                if (texture->has_alpha()) {
                    mask_tex_ = texture;
                    mask_ = add_material("material::mask", "mask", {});
                    if (mask_ < 0) {
                        LM_THROW_EXCEPTION_DEFAULT(Error::InvalidArgument);
                    }
                }
            }
        }
    }

    virtual bool is_specular(const PointGeometry& geom, int comp) const override {
        return materials_.at(comp)->is_specular(geom, SurfaceComp::DontCare);
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
            const auto wd = diffuse_selection_weight(geom);
            if (rng.u() < wd) {
                if (mask_tex_ && rng.u() > mask_tex_->eval_alpha(geom.t)) {
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

    virtual std::optional<Vec3> sample_direction_given_comp(Rng& rng, const PointGeometry& geom, int comp, Vec3 wi) const override {
        return materials_.at(comp)->sample_direction_given_comp(rng, geom, SurfaceComp::DontCare, wi);
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

    virtual Float pdf_comp(const PointGeometry& geom, int comp, Vec3) const override {
        if (comp == glass_ || comp == mirror_) {
            return 1_f;
        }
        const auto wd = diffuse_selection_weight(geom);
        return comp == glossy_ ? (1_f - wd) : wd;
    }

    virtual Vec3 eval(const PointGeometry& geom, int comp, Vec3 wi, Vec3 wo) const override {
        return materials_.at(comp)->eval(geom, SurfaceComp::DontCare, wi, wo);
    }

private:
    Float diffuse_selection_weight(const PointGeometry& geom) const {
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
        return wd;
    }
};

LM_COMP_REG_IMPL(Material_WavefrontObj, "material::wavefrontobj");

LM_NAMESPACE_END(LM_NAMESPACE)
