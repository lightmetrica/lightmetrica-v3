/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>
using namespace lm::literals;

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// Simple ambient occlusion renderer
class Renderer_AO final : public Renderer {
private:
    Scene* scene_;
    Film* film_;
    long long spp_;
    int rng_seed_ = 42;

public:
    virtual void construct(const Json& prop) override {
        scene_ = json::comp_ref<Scene>(prop, "scene");
        film_ = json::comp_ref<Film>(prop, "output");
        spp_ = json::value<long long>(prop, "spp");
    }

    virtual Json render() const override {
        const auto size = film_->size();
        parallel::foreach(size.w*size.h, [&](long long index, int threadId) -> void {
            thread_local Rng rng(rng_seed_ + threadId);
            const int x = int(index % size.w);
            const int y = int(index / size.w);
            const auto ray = path::primary_ray(scene_, {(x+.5_f)/size.w, (y+.5_f)/size.h});
            const auto hit = scene_->intersect(ray);
            if (!hit) {
                return;
            }
            auto V = 0_f;
            for (long long i = 0; i < spp_; i++) {
                const auto [n, u, v] = hit->geom.orthonormal_basis_twosided(-ray.d);
                const auto d = math::sample_cosine_weighted(rng.next<Vec2>());
                V += scene_->intersect({hit->geom.p, u*d.x+v*d.y+n*d.z}, Eps, .2_f) ? 0_f : 1_f;
            }
            V /= spp_;
            film_->set_pixel(x, y, Vec3(V));
        });
        return {};
    }
};

LM_COMP_REG_IMPL(Renderer_AO, "renderer::ao");

LM_NAMESPACE_END(LM_NAMESPACE)

// ------------------------------------------------------------------------------------------------

// This example illustrates how to create custom renderer.
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
        const auto opt = lm::json::parse_positional_args<13>(argc, argv, R"({{
            "obj": "{}",
            "out": "{}",
            "spp": {},
            "w": {},
            "h": {},
            "eye": [{},{},{}],
            "lookat": [{},{},{}],
            "vfov": {}
        }})");

        // ----------------------------------------------------------------------------------------

        // Define assets

        // Film for the rendered image
        const auto* film = lm::load<lm::Film>("film1", "film::bitmap", {
            {"w", opt["w"]},
            {"h", opt["h"]}
        });

        // Pinhole camera
        const lm::Float w = opt["w"];
        const lm::Float h = opt["h"];
        const auto* camera = lm::load<lm::Camera>("camera1", "camera::pinhole", {
            {"film", film->loc()},
            {"position", opt["eye"]},
            {"center", opt["lookat"]},
            {"up", {0,1,0}},
            {"vfov", opt["vfov"]},
            {"aspect", w / h}
        });

        // OBJ model
        // Replace all materials to diffuse and use our checker texture
        const auto* model = lm::load<lm::Model>("obj1", "model::wavefrontobj", {
            {"path", opt["obj"]}
        });

        // ----------------------------------------------------------------------------------------

        // Define scene primitives
        const auto* accel = lm::load<lm::Accel>("accel", "accel::sahbvh", {});
        auto* scene = lm::load<lm::Scene>("scene", "scene::default", {
            {"accel", accel->loc()}
        });
        
        // Camera
        scene->add_primitive({
            {"camera", camera->loc()}
        });

        // Create primitives from model asset
        scene->add_primitive({
            {"model", model->loc()}
        });

        // Build acceleration structure
        scene->build();

        // ----------------------------------------------------------------------------------------

        // Render an image
        const auto* renderer = lm::load<lm::Renderer>("renderer", "renderer::ao", {
            {"output", film->loc()},
            {"scene", scene->loc()},
            {"spp", opt["spp"]}
        });
        renderer->render();

        // Save rendered image
        film->save(opt["out"]);

        // Shutdown the framework
        lm::shutdown();
    }
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }

    return 0;
}
