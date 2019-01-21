/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>

/*
    This example illustrates how to get information of the object tree.
    Command line arguments are same as `raycast.cpp`.
*/
int main(int argc, char** argv) {
    try {
        // Initialize the framework
        lm::init("user::default", {
            #if LM_DEBUG_MODE
            {"numThreads", -1}
            #else
            {"numThreads", -1}
            #endif
            });

        // Parse command line arguments
        const auto opt = lm::json::parsePositionalArgs<11>(argc, argv, R"({{
            "obj": "{}",
            "out": "{}",
            "w": {},
            "h": {},
            "eye": [{},{},{}],
            "lookat": [{},{},{}],
            "vfov": {}
        }})");

        // --------------------------------------------------------------------
        
        // Define assets and primitives
        #if LM_DEBUG_MODE
        // Load internal state
        lm::deserialize("lm.serialized");
        #else
        // Film for the rendered image
        lm::asset("film1", "film::bitmap", {
            {"w", opt["w"]},
            {"h", opt["h"]}
        });

        // Pinhole camera
        lm::asset("camera1", "camera::pinhole", {
            {"film", "film1"},
            {"position", opt["eye"]},
            {"center", opt["lookat"]},
            {"up", {0,1,0}},
            {"vfov", opt["vfov"]}
        });

        // OBJ model
        lm::asset("obj1", "model::wavefrontobj", {
            {"path", opt["obj"]}
        });

        // Camera
        lm::primitive(lm::Mat4(1), { {"camera", "camera1"} });

        // Create primitives from model asset
        lm::primitives(lm::Mat4(1), "obj1");

        // Build acceleration structure
        lm::build("accel::sahbvh");

        // Save internal state for the debug mode
        lm::serialize("lm.serialized");
        #endif

        // --------------------------------------------------------------------

        // Prints all registered components
        LM_INFO("Registered components");
        lm::comp::detail::foreachRegistered([&](const std::string& name) {
            LM_INFO("- {}", name);
        });

        // --------------------------------------------------------------------

        // Prints object hierarchy of the framework
        LM_INFO("Component hierarchy");
        const lm::Component::ComponentVisitor visit = [&](lm::Component* comp, bool weak) {
            if (!comp) {
                LM_INFO("- nullptr");
                return;
            }
            if (!weak) {
                LM_INFO("- unique [key='{}', loc='{}']", comp->key(), comp->loc());
                LM_INDENT();
                comp->foreachUnderlying(visit);
            }
            else {
                LM_INFO("-> weak [key='{}', loc='{}']", comp->key(), comp->loc());
            }
        };
        lm::comp::detail::foreachComponent(visit);

        // --------------------------------------------------------------------

        // APIs for getting underlying information of a component
        
        // lm::comp::get() function can access component tree from the root component
        // Root component is currently `user::default` and it has three underlying components.
        auto* assets = lm::comp::get<lm::Assets>("assets");

        // Component::underlying() function can access underlying component.
        auto* obj1 = assets->underlying<lm::Model>("obj1");

        // Alternatively, the same component can be accessed
        // directly by lm::comp::get() function with 'xxx.yyy.zzz' format.
        auto* obj2 = lm::comp::get<lm::Model>("assets.obj1");
        LM_UNUSED(obj2);
        assert(obj1 == obj2);
            
        // We can iterate the underlying assets with Component::foreachUnderlying() function.
        obj1->foreachUnderlying([](lm::Component* p, bool) {
            // The implementation key of underlying assets can be
            // obtained by Component::key() function.
            const auto key = p->key();
            if (key == "material::wavefrontobj") {
                LM_INFO("Material");
                LM_INDENT();

                // Some component supports implementation-specific getter function
                // where the values are serialized to Json format.
                // For instance, `material::wavefrontobj` exposes underlying material parameters
                // for the corresponding MLT file.
                const auto v = p->underlyingValue();
                for (const auto& e : v.items()) {
                    LM_INFO("{}: {}", e.key(), e.value().dump());
                }
            }
        });
    }
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }

    return 0;
}
