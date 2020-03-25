/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/material.h>
#include <lm/surface.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Material_ConstantWeightMixture_RR final : public Material {
private:
    struct Entry {
        Material* material;
        Float weight;
    };
    std::vector<Entry> materials_;
    Dist dist_;

public:
    virtual void construct(const Json& prop) override {
        for (auto& entry : prop) {
            auto* mat = json::comp_ref<Material>(entry, "material");
            const auto weight = json::value<Float>(entry, "weight");
            materials_.push_back({ mat, weight });
            dist_.add(weight);
        }
        dist_.norm();
    }
    
    virtual ComponentSample sample_component(const ComponentSampleU& u, const PointGeometry&) const override {
        const int comp = dist_.sample(u.uc[0]);
        const auto p = dist_.pmf(comp);
        return { comp, 1_f / p };
    }

    virtual Float pdf_component(int comp, const PointGeometry&) const override {
        return dist_.pmf(comp);
    }

    virtual std::optional<DirectionSample> sample_direction(const DirectionSampleU& us, const PointGeometry& geom, Vec3 wi, int comp, TransDir trans_dir) const override {
        const auto& e = materials_[comp];
        const auto s = e.material->sample_direction(us, geom, wi, {}, trans_dir);
        if (!s) {
            return {};
        }
        return DirectionSample{
            s->wo,
            e.weight * s->weight
        };
    }

    virtual Vec3 reflectance(const PointGeometry& geom) const override {
        Vec3 sum(0_f);
        for (const auto& e : materials_) {
            sum += e.weight * e.material->reflectance(geom);
        }
        return sum;
    }

    virtual Float pdf_direction(const PointGeometry& geom, Vec3 wi, Vec3 wo, int comp, bool eval_delta) const override {
        return materials_[comp].material->pdf_direction(geom, wi, wo, {}, eval_delta);
    }

    virtual Vec3 eval(const PointGeometry& geom, Vec3 wi, Vec3 wo, int comp, TransDir trans_dir, bool eval_delta) const override {
        const auto& e = materials_[comp];
        return e.weight * e.material->eval(geom, wi, wo, {}, trans_dir, eval_delta);
    }

    virtual bool is_specular_component(int comp) const override {
        return materials_[comp].material->is_specular_component({});
    }
};

LM_COMP_REG_IMPL(Material_ConstantWeightMixture_RR, "material::constant_weight_mixture_rr");

// ------------------------------------------------------------------------------------------------

class Material_ConstantWeightMixture_Marginalized final : public Material {
private:
    struct MaterialGroup {
        struct Entry {
            Material* material;
            Float weight;
        };
        std::vector<Entry> entries;
        Dist dist;
    };
    std::vector<MaterialGroup> material_groups_;
    Dist dist_;

public:
    virtual void construct(const Json& prop) override {
        // Create default material group for non-specular materials
        material_groups_.emplace_back();

        // Load entries
        for (auto& entry : prop) {
            auto* mat = json::comp_ref<Material>(entry, "material");
            const auto weight = json::value<Float>(entry, "weight");
            if (mat->is_specular_component({})) {
                // If the specular material is given, create a new material group.
                material_groups_.emplace_back();
                material_groups_.back().entries.push_back({ mat, weight });
            }
            else {
                // Otherwise, add the entry to the default group.
                material_groups_[0].entries.push_back({ mat, weight });
            }
        }

        // Comput distributions for component selection
        for (auto& group : material_groups_) {
            Float weight_sum = 0_f;
            for (const auto& entry : group.entries) {
                weight_sum += entry.weight;
                group.dist.add(entry.weight);
            }
            group.dist.norm();
            dist_.add(weight_sum);
        }
        dist_.norm();
    }

    virtual ComponentSample sample_component(const ComponentSampleU& u, const PointGeometry&) const override {
        const int comp = dist_.sample(u.uc[0]);
        const auto p = dist_.pmf(comp);
        return { comp, 1_f / p };
    }

    virtual Float pdf_component(int comp, const PointGeometry&) const override {
        return dist_.pmf(comp);
    }

    virtual std::optional<DirectionSample> sample_direction(const DirectionSampleU& us, const PointGeometry& geom, Vec3 wi, int comp, TransDir trans_dir) const override {
        const auto& group = material_groups_[comp];
        
        // Sample component in the selected group
        const int comp_in_group = group.dist.sample(us.udc[0]);
        const auto& e = group.entries[comp_in_group];

        // Sample direction
        const auto s = e.material->sample_direction(us, geom, wi, {}, trans_dir);
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

    virtual Vec3 reflectance(const PointGeometry& geom) const override {
        Vec3 sum(0_f);
        for (auto& group : material_groups_) {
            for (const auto& entry : group.entries) {
                sum += entry.weight * entry.material->reflectance(geom);
            }
        }
        return sum;
    }

    virtual Float pdf_direction(const PointGeometry& geom, Vec3 wi, Vec3 wo, int comp, bool eval_delta) const override {
        // Compute marginal PDF
        const auto& group = material_groups_[comp];
        Float p_marginal = 0_f;
        for (int i = 0; i < (int)(group.entries.size()); i++) {
            const auto p_sel = group.dist.pmf(i);
            const auto p = group.entries[i].material->pdf_direction(geom, wi, wo, {}, eval_delta);
            p_marginal += p_sel * p;
        }
        return p_marginal;
    }

    virtual Vec3 eval(const PointGeometry& geom, Vec3 wi, Vec3 wo, int comp, TransDir trans_dir, bool eval_delta) const override {
        const auto& group = material_groups_[comp];
        Vec3 result(0_f);
        for (const auto& entry : group.entries) {
            const auto f = entry.material->eval(geom, wi, wo, {}, trans_dir, eval_delta);
            result += entry.weight * f;
        }
        return result;
    }

    virtual bool is_specular_component(int comp) const override {
        // Default group : non-specular. Others: specular
        return comp != 0;
    }
};

LM_COMP_REG_IMPL(Material_ConstantWeightMixture_Marginalized, "material::constant_weight_mixture_marginalized");

LM_NAMESPACE_END(LM_NAMESPACE)
