/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include "lmgl.h"

/*
    Visual debugger using debugio subsystem.
*/
int main() {
    try {
        // Initialize the framework
        lm::ScopedInit init("user::default", {
            {"debugio", {
                {"debugio::server", {
                    {"port", 5555}
                }}
            }}
        });

        // --------------------------------------------------------------------

        InteractiveApp app;
        if (!app.setup({
            {"w", 1920},
            {"h", 1080},
            {"eye", {0,0,1}},
            {"lookat", {0,0,0}},
            {"vfov", 30}
        })) {
            THROW_RUNTIME_ERROR();
        }

        std::atomic<bool> doSync = false;
        lm::debugio::server::on_syncUserContext([&]() {
            doSync = true;
        });

        std::thread([]() {
            lm::debugio::server::run();
        }).detach();

        // --------------------------------------------------------------------

        app.run([&](int display_w, int display_h) {
            // Synchronize user context with client
            if (doSync) {
                LM_INFO("Syncing user context");

                // Reset OpenGL scene
                app.glscene.reset();

                // Create OpenGL-ready assets and register primitives
                const auto* scene = lm::comp::get<lm::Scene>("scene");
                scene->foreachPrimitive([&](const lm::Primitive& p) {
                    if (p.camera) {
                        // Reset camera parameters
                        const auto cameraParams = p.camera->underlyingValue();
                        app.glcamera.reset(
                            cameraParams["eye"],
                            cameraParams["center"],
                            cameraParams["up"],
                            cameraParams["vfov"]);
                        return;
                    }
                    if (!p.mesh || !p.material) {
                        return;
                    }
                    app.glscene.add(p.transform.M, p.mesh, p.material);
                });
                doSync = false;
            }

            // ------------------------------------------------------------

            // Renderer configuration
            ImGui::Begin("Renderer configuration");
            static int spp = 10;
            static int maxLength = 20;
            ImGui::SliderInt("spp", &spp, 1, 1000);
            ImGui::SliderInt("maxLength", &maxLength, 1, 100);

            // Dispatch rendering
            static std::atomic<bool> rendering = false;
            static std::atomic<bool> renderingFinished = false;
            if (ImGui::ButtonEx("Render [R]", ImVec2(0, 0), rendering ? ImGuiButtonFlags_Disabled : 0) || ImGui::IsKeyReleased('R')) {
                // Create film to store rendered image
                lm::asset("film_render", "film::bitmap", {
                    {"w", display_w},
                    {"h", display_h}
                });

                // Camera
                lm::asset("camera_render", "camera::pinhole", {
                    {"film", "film_render"},
                    {"position", app.glcamera.eye()},
                    {"center", app.glcamera.center()},
                    {"up", {0,1,0}},
                    {"vfov", app.glcamera.fov()}
                });

                // Renderer
                lm::renderer("renderer::pt", {
                    {"output", "film_render"},
                    {"spp", spp},
                    {"maxLength", maxLength}
                });

                // Create a new thread and dispatch rendering
                rendering = true;
                std::thread([]() {
                    lm::render(true);
                    rendering = false;
                    renderingFinished = true;
                }).detach();
            }

            ImGui::End();

            // ------------------------------------------------------------
                
            // Checking rendered image
            static std::optional<GLuint> texture;
            const auto updateTexture = [&]() {
                // Underlying data
                auto [w, h, data] = lm::buffer("film_render");

                // Convert data to float
                std::vector<float> data_(w*h * 3);
                for (int i = 0; i < w*h * 3; i++) {
                    data_[i] = float(data[i]);
                }

                // Load as OpenGL texture
                if (!texture) {
                    texture = 0;
                    glGenTextures(1, &*texture);
                }
                glBindTexture(GL_TEXTURE_2D, *texture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_FLOAT, data_.data());
                glBindTexture(GL_TEXTURE_2D, 0);

                CHECK_GL_ERROR();
            };
            if (rendering) {
                using namespace std::chrono;
                using namespace std::literals::chrono_literals;
                const auto now = high_resolution_clock::now();
                static auto lastupdated = now;
                const auto delay = 100ms;
                const auto elapsed = duration_cast<milliseconds>(now - lastupdated);
                if (elapsed > delay) {
                    updateTexture();
                    lastupdated = now;
                }
            }
            if (renderingFinished) {
                updateTexture();
                renderingFinished = false;
            }
            if (texture) {
                // Update rendered image
                int w, h;
                glBindTexture(GL_TEXTURE_2D, *texture);
                glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
                glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
                glBindTexture(GL_TEXTURE_2D, 0);

                ImGui::SetNextWindowSize(ImVec2(float(w/2), float(h/2+40)), ImGuiSetCond_Once);
                ImGui::Begin("Rendered");

                // Display texture
                #pragma warning(push)
                #pragma warning(disable:4312)
                ImGui::Image((ImTextureID*)*texture, ImVec2(float(w/2), float(h/2)), ImVec2(0, 1), ImVec2(1, 0),
                    ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128));
                #pragma warning(pop)

                ImGui::End();
            }
        });
    }
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }

    return EXIT_SUCCESS;
}
