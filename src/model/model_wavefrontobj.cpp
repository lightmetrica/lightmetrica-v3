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

#define NO_MIXTURE_MATERIAL 0

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

using namespace objloader;
class Mesh_WavefrontObjRef;

/*
\rst
.. function:: model::wavefrontobj

    Wavefront OBJ model.

    :param str path: Path to ``.obj`` file.
\endrst
*/
class Model_WavefrontObj final : public Model {
private:
    friend class Mesh_WavefrontObjRef;

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
        const bool result = objloader::load(path, geo_,
            // Process mesh
            [&](const OBJMeshFace& fs, const MTLMatParams& m) -> bool {
                // Create mesh
                const std::string mesh_name = std::format("mesh_{}", assets_.size());
                auto mesh = comp::create<Mesh>(
                    "mesh::wavefrontobj_ref", make_loc(mesh_name),
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

                // Create mesh group
                groups_.push_back({ assets_map_[mesh_name], assets_map_[m.name], light_index });

                return true;
            },

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

                // Load material
                Ptr<Material> mat;
                const bool skip_specular_mat = json::value<bool>(prop, "skip_specular_mat", false);
                if (m.illum == 5 || m.illum == 7) {
                    if (skip_specular_mat) {
                        // Skip specular material. Use black material.
                        mat = comp::create<Material>(
                            "material::diffuse", make_loc(m.name), { {"Kd", Vec3(0_f)} });
                    }
                    else if (m.illum == 7) {
                        // Glass
                        mat = comp::create<Material>(
                            "material::glass", make_loc(m.name), { {"Ni", m.Ni} });
                    }
                    else if (m.illum == 5) {
                        // Mirror
                        mat = comp::create<Material>(
                            "material::mirror", make_loc(m.name), {});
                    }
                }
                else {

                    #if NO_MIXTURE_MATERIAL
                    if (math::is_zero(m.Ks)) {
                        // Diffuse material
                        mat = comp::create<Material>(
                            "material::diffuse", make_loc(m.name), {
                                {"Kd", m.Kd},
                                {"mapKd", mapKd_loc}
                            });
                    }
                    else {
                        // Glossy material
                        const auto r = 2_f / (2_f + m.Ns);
                        const auto as = math::safe_sqrt(1_f - m.an * .9_f);
                        mat = comp::create<Material>(
                            "material::glossy", make_loc(m.name), {
                                {"Ks", m.Ks},
                                {"ax", std::max(1e-3_f, r / as)},
                                {"ay", std::max(1e-3_f, r * as)}
                            });
                        
                    }
                    #else
                    // Convert parameter for anisotropic GGX 
                    const auto r = 2_f / (2_f + m.Ns);
                    const auto as = math::safe_sqrt(1_f - m.an * .9_f);

                    // Default mixture material of D and G
                    mat = comp::create<Material>(
                        "material::mixture_wavefrontobj",
                        make_loc(m.name),
                        {
                            {"Kd", m.Kd},
                            {"mapKd", mapKd_loc},
                            {"Ks", m.Ks},
                            {"ax", std::max(1e-3_f, r / as)},
                            {"ay", std::max(1e-3_f, r * as)},
                            {"no_alpha_mask", skip_specular_mat}
                        });
                    #endif
                }
                if (!mat) {
                    return false;
                }
                assets_map_[m.name] = int(assets_.size());
                assets_.push_back(std::move(mat));

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

/*
\rst2
.. function:: mesh::wavefrontobj_ref

    Mesh for Wavefront OBJ model.

    This asset is internally used by the framework.
\endrst2
*/
class Mesh_WavefrontObjRef final  : public Mesh {
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

    virtual void foreach_triangle(const ProcessTriangleFunc& process_triangle) const override {
        for (int fi = 0; fi < int(fs_.size())/3; fi++) {
            process_triangle(fi, triangle_at(fi));
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

    virtual InterpolatedPoint surface_point(int face, Vec2 uv) const override {
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
            i1.n < 0 ? math::geometry_normal(p1, p2, p3)
                     : glm::normalize(math::mix_barycentric(
                         geo_.ns[i1.n], geo_.ns[i2.n], geo_.ns[i3.n], uv)),
            // Geometry normal
            math::geometry_normal(p1, p2, p3),
            // Texture coordinates
            i1.t < 0 ? Vec2(0) : math::mix_barycentric(
                geo_.ts[i1.t], geo_.ts[i2.t], geo_.ts[i3.t], uv)
        };
    }

    virtual int num_triangles() const override {
        return int(fs_.size()) / 3;
    }
};

LM_COMP_REG_IMPL(Mesh_WavefrontObjRef, "mesh::wavefrontobj_ref");

// ------------------------------------------------------------------------------------------------

/*
\rst3
.. function:: material::mixture_wavefrontobj

    Mixture material for Wavefront OBJ model.

    This asset is internally used by the framework.
\endrst3
*/
class Material_Mixture_WavefrontObj final : public Material {
private:
    Component::Ptr<Material> diffuse_;
    Component::Ptr<Material> glossy_;
    Texture* mask_tex_ = nullptr;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(diffuse_, glossy_, mask_tex_);
    }

    virtual void foreach_underlying(const ComponentVisitor& visit) override {
        comp::visit(visit, diffuse_);
        comp::visit(visit, glossy_);
        comp::visit(visit, mask_tex_);
    }

    virtual Component* underlying(const std::string& name) const override {
        if (name == "diffuse")
            return diffuse_.get();
        else if (name == "glossy")
            return glossy_.get();
        return nullptr;
    }

private:
    // Evaluate alpha value
    Float eval_alpha(const PointGeometry& geom) const {
        return !mask_tex_ ? 1_f : mask_tex_->eval_alpha(geom.t);
    }

    // Get material by index
    const Material* material_by_index(int index) const {
        if (index == 0)
            return diffuse_.get();
        else if (index == 1)
            return glossy_.get();
        return nullptr;
    }

public:
    virtual void construct(const Json& prop) override {
        // Create underlying materials
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
        const bool no_alpha_mask = json::value<bool>(prop, "no_alpha_mask");
        if (!no_alpha_mask && !mapKd.empty()) {
            auto* texture = comp::get<Texture>(mapKd);
            if (texture && texture->has_alpha()) {
                mask_tex_ = texture;
            }
        }
    }

    virtual ComponentSample sample_component(const ComponentSampleU& u, const PointGeometry& geom, Vec3) const override {
        // Sample component group index
        const auto alpha = eval_alpha(geom);
        if (u.uc[0] < alpha) {
            return { 0, 1_f / alpha };
        }
        return { 1, 1_f / (1_f - alpha) };
    }

    virtual Float pdf_component(int comp, const PointGeometry& geom, Vec3) const override {
        const auto alpha = eval_alpha(geom);
        return comp == 0 ? alpha : (1_f - alpha);
    }

    Float diffuse_selection_prob(const PointGeometry& geom) const {
        auto wd = glm::compMax(diffuse_->reflectance(geom));
        auto wg = glm::compMax(glossy_->reflectance(geom));
        if (wd == 0_f && wg == 0_f) {
            return 1_f;
        }
        const auto w = wd / (wd + wg);
        return w;
    }

    virtual std::optional<DirectionSample> sample_direction(const DirectionSampleU& us, const PointGeometry& geom, Vec3 wi, int comp, TransDir trans_dir) const override {
        if (comp == 0) {
            // Diffuse or glossy
            // Sample omponent (diffuse=0, glossy=1)
            const int comp_in_group = [&]() {
                const auto w = diffuse_selection_prob(geom);
                if (us.udc[0] < w) {
                    return 0;
                }
                return 1;
            }();

            // Sample direction
            const auto* material = material_by_index(comp_in_group);
            const auto s = material->sample_direction(us, geom, wi, {}, trans_dir);
            if (!s) {
                return {};
            }
            
            // Evaluate weight
            const auto f = eval(geom, wi, s->wo, comp, trans_dir, false);
            const auto p = pdf_direction(geom, wi, s->wo, comp, false);
            const auto C = f / p;

            return DirectionSample{
                s->wo,
                C
            };
        }
        else {
            // Alpha mask
            const auto wo = -wi;
            const auto f = eval(geom, wi, wo, comp, trans_dir, false);
            const auto p = pdf_direction(geom, wi, wo, comp, false);
            const auto C = f / p;
            return DirectionSample{
                wo,
                Vec3(1_f)
            };
        }
    }

    virtual Vec3 reflectance(const PointGeometry& geom) const override {
        // Just use diffuse component
        return diffuse_->reflectance(geom);
    }

    virtual Float pdf_direction(const PointGeometry& geom, Vec3 wi, Vec3 wo, int comp, bool eval_delta) const override {
        if (comp == 0) {
            Float p_marginal = 0_f;
            const auto w = diffuse_selection_prob(geom);
            p_marginal += w * diffuse_->pdf_direction(geom, wi, wo, {}, eval_delta);
            p_marginal += (1_f-w) * glossy_->pdf_direction(geom, wi, wo, {}, eval_delta);
            return p_marginal;
        }
        else {
            return eval_delta ? 0_f : 1_f;
        }
    }

    virtual Vec3 eval(const PointGeometry& geom, Vec3 wi, Vec3 wo, int comp, TransDir trans_dir, bool eval_delta) const override {
        const auto alpha = eval_alpha(geom);
        if (comp == 0) {
            Vec3 sum(0_f);
            sum += diffuse_->eval(geom, wi, wo, {}, trans_dir, eval_delta);
            sum += glossy_->eval(geom, wi, wo, {}, trans_dir, eval_delta);
            return sum * alpha;
        }
        else {
            return eval_delta ? Vec3(0_f) : Vec3(1_f - alpha);
        }
    }

    virtual bool is_specular_component(int comp) const override {
        return comp != 0;
    }
};

LM_COMP_REG_IMPL(Material_Mixture_WavefrontObj, "material::mixture_wavefrontobj");

LM_NAMESPACE_END(LM_NAMESPACE)
