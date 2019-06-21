/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>
#include <iostream>

// ----------------------------------------------------------------------------

class Material_VisualizeNormal final : public lm::Material {
public:
    virtual bool construct(const lm::Json&) override {
        return true;
    }

    virtual bool isSpecular(const lm::PointGeometry&, int) const override {
        return false;
    }

    virtual std::optional<lm::MaterialDirectionSample> sample(lm::Rng&, const lm::PointGeometry&, lm::Vec3) const override {
        LM_UNREACHABLE_RETURN();
    }

    virtual std::optional<lm::Vec3> reflectance(const lm::PointGeometry& geom, int) const override {
        return glm::abs(geom.n);
    }

    virtual lm::Float pdf(const lm::PointGeometry&, int, lm::Vec3, lm::Vec3) const override {
        LM_UNREACHABLE_RETURN();
    }

    virtual lm::Vec3 eval(const lm::PointGeometry&, int, lm::Vec3, lm::Vec3) const override {
        LM_UNREACHABLE_RETURN();
    }
};

LM_COMP_REG_IMPL(Material_VisualizeNormal, "material::visualize_normal");

// ----------------------------------------------------------------------------

/*
    This example illustrates how to extend features of Lightmetrica
    by creating simple material extension.
    Command line arguments are same as `raycast.cpp`.
*/
int main(int argc, char** argv) {
    try {
        // Initialize the framework
        lm::init();
        lm::parallel::init(lm::parallel::DefaultType, {
            #if LM_DEBUG_MODE
            {"numThreads", 1}
            #else
            {"numThreads", -1}
            #endif
        });
        lm::info();

        // Parse command line arguments
        const auto opt = lm::json::parsePositionalArgs<13>(argc, argv, R"({{
            "obj": "{}",
            "out": "{}",
            "w": {},
            "h": {},
            "eye": [{},{},{}],
            "lookat": [{},{},{}],
            "vfov": {}
        }})");

        // --------------------------------------------------------------------

        // Define assets

        // Film for the rendered image
        lm::asset("film1", "film::bitmap", {
            {"w", opt["w"]},
            {"h", opt["h"]}
        });

        // Pinhole camera
        lm::asset("camera1", "camera::pinhole", {
            {"film", lm::asset("film1")},
            {"position", opt["eye"]},
            {"center", opt["lookat"]},
            {"up", {0,1,0}},
            {"vfov", opt["vfov"]}
        });

        // Material to replace
        lm::asset("visualize_normal_mat", "material::visualize_normal", {});

        // OBJ model
        // Replace all materials to diffuse and use our checker texture
        lm::asset("obj1", "model::wavefrontobj", {
            {"path", opt["obj"]},
            {"base_material", lm::asset("visualize_normal_mat")}
        });

        // --------------------------------------------------------------------

        // Define scene primitives

        // Camera
        lm::primitive(lm::Mat4(1), {
            {"camera", lm::asset("camera1")}
        });

        // Create primitives from model asset
        lm::primitive(lm::Mat4(1), {
            {"model", lm::asset("obj1")}
        });

        // --------------------------------------------------------------------

        // Render an image
        lm::build("accel::sahbvh");
        lm::render("renderer::raycast", {
            {"output", lm::asset("film1")},
            {"use_constant_color", true}
        });

        // Save rendered image
        lm::save(lm::asset("film1"), opt["out"]);

        // Shutdown the framework
        lm::shutdown();
    }
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }

    return 0;
}
