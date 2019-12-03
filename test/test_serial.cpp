/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include "test_common.h"
#include <lm/serial.h>
#include <lm/math.h>

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

// ----------------------------------------------------------------------------

// Type trait to check the type are comparable.
template <typename T, typename = void>
struct is_equality_comparable : std::false_type {};
template <typename T>
struct is_equality_comparable<
    T,
    std::enable_if_t<
        std::is_convertible_v<
            decltype(std::declval<T>() == std::declval<T>()),
            bool
        >
    >
> : std::true_type {};

/*
    Check serialization and deserialization work correctly.
    Instead of comparing the variables orig and loaded
    we compare the serialized value of loaded because
    the type of orig or loaded might not have operator==().
*/
template <typename T>
void check_save_and_load_round_trip_compare_values(T&& orig) {
    // Serialize the input value
    std::stringstream ss1;
    lm::serial::save(ss1, std::forward<T>(orig));

    // Deserialize it
    std::decay_t<T> loaded;
    lm::serial::load(ss1, loaded);

    // If T is equality-comparable, check orig == loaded
    if constexpr (is_equality_comparable<T>::value) {
        CHECK(orig == loaded);
    }
}

template <typename T>
void check_save_and_load_round_trip_compare_loaded(T&& orig) {
    // Serialize the input value
    std::stringstream ss1;
    lm::serial::save(ss1, std::forward<T>(orig));
    const auto s1 = ss1.str();

    // Dirty hack: Repeat 4 times
    auto s2 = s1;
    for (int i = 0; i < 4; i++) {
        // Deserialize it
        std::stringstream ss2(s2);
        std::decay_t<T> loaded;
        lm::serial::load(ss2, loaded);

        // Serialize again with different stream
        std::stringstream ss3;
        lm::serial::save(ss3, loaded);
        s2 = ss3.str();
    }

    // Compare two streams
    CHECK(s1 == s2);
}

template <typename T>
void check_save_and_load_round_trip(T&& orig) {
    check_save_and_load_round_trip_compare_values(std::forward<T>(orig));
    check_save_and_load_round_trip_compare_loaded(std::forward<T>(orig));
}

// ------------------------------------------------------------------------------------------------

struct TestSerial_SimpleStruct {
    int v1;
    int v2;
    template <typename Archive>
    void serialize(Archive& ar) {
        ar(v1, v2);
    }
};

struct TestSerial_SimpleNestedStruct {
    std::vector<TestSerial_SimpleStruct> vs;
    template <typename Archive>
    void serialize(Archive& ar) {
        ar(vs);
    }
};

struct TestSerial_Simple final : public lm::Component {
    int v1;
    int v2;
    virtual void construct(const lm::Json& prop) override {
        v1 = prop["v1"];
        v2 = prop["v2"];
    }
    LM_SERIALIZE_IMPL(ar) {
        ar(v1, v2);
    }
};

LM_COMP_REG_IMPL(TestSerial_Simple, "testserial_simple");

struct TestSerial_Nested final : public lm::Component {
    Component::Ptr<Component> p;
    TestSerial_Nested() {
        p = lm::comp::create<Component>("testserial_simple", "", {
            { "v1", 42 },
            { "v2", 32 }
        });
    }
    LM_SERIALIZE_IMPL(ar) {
        ar(p);
    }
};

LM_COMP_REG_IMPL(TestSerial_Nested, "testserial_nested");

struct TestSerial_Ref final : public lm::Component {
    Component* p;
    virtual void construct(const lm::Json& prop) {
        p = lm::comp::get<Component>(prop["ref"]);
    }
    LM_SERIALIZE_IMPL(ar) {
        ar(p);
    }
};

LM_COMP_REG_IMPL(TestSerial_Ref, "testserial_ref");

struct TestSerial_Container final : public lm::Component {
    std::vector<Component::Ptr<Component>> v;
    std::unordered_map<std::string, int> m;

    // Add a component to the container
    void add(const std::string& name, const std::string& key, const lm::Json& prop) {
        auto p = lm::comp::create<lm::Component>(key, make_loc(loc(), name), prop);
        CHECK(p);
        m[name] = int(v.size());
        v.push_back(std::move(p));
    }

    virtual Component* underlying(const std::string& name) const {
        return v[m.at(name)].get();
    }

    LM_SERIALIZE_IMPL(ar) {
        // Order of the argument is critical
        ar(m, v);
    }
};

LM_COMP_REG_IMPL(TestSerial_Container, "testserial_container");

// Imitating root component
struct TestSerial_Root final : public lm::Component {
    // Underlying component
    Component::Ptr<Component> p;

    TestSerial_Root() {
        // Register this component as root
        lm::comp::detail::Access::loc(this) = "$";
        lm::comp::detail::register_root_comp(this);
    }

    virtual Component* underlying(const std::string& name) const {
        if (name == "p") {
            return p.get();
        }
        return nullptr;
    }

    // Clear state. Returns old underlying component.
    Component::Ptr<Component> clear() {
        return Component::Ptr<Component>(p.release());
    }

    // Save current state to string
    std::string saveState() const {
        std::stringstream ss;
        lm::serial::save(ss, p);
        return ss.str();
    }

    // Load state from string
    void loadState(const std::string& state) {
        std::stringstream ss(state);
        lm::serial::load(ss, p);
    }
};

// ------------------------------------------------------------------------------------------------

TEST_CASE("Serialization") {
    lm::log::ScopedInit init;
    
    SUBCASE("Primitive types") {
        check_save_and_load_round_trip<bool>(true);
        check_save_and_load_round_trip<int>(42);
        check_save_and_load_round_trip<double>(42.0);
        check_save_and_load_round_trip<float>(42.f);
        check_save_and_load_round_trip<std::string>("hai domo");
    }

    SUBCASE("Vector and matrix") {
        check_save_and_load_round_trip<lm::Vec2>(lm::Vec2(1,2));
        check_save_and_load_round_trip<lm::Vec3>(lm::Vec3(1,2,3));
        check_save_and_load_round_trip<lm::Vec4>(lm::Vec4(1,2,3,4));
        check_save_and_load_round_trip<lm::Mat3>(lm::Mat3(1,2,3,4,5,6,7,8,9));
        check_save_and_load_round_trip<lm::Mat4>(lm::Mat4(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16));
    }

    SUBCASE("STL containers") {
        check_save_and_load_round_trip<std::vector<int>>({ 1,2,3,4,5 });
        check_save_and_load_round_trip<std::unordered_map<std::string, int>>({
            { "A", 1 },
            { "B", 2 },
            { "C", 3 },
            { "D", 4 }
        });
    }

    SUBCASE("Simple struct") {
        SUBCASE("Simple") {
            TestSerial_SimpleStruct orig{ 42, 43 };
            check_save_and_load_round_trip(orig);
        }
        SUBCASE("Nested") {
            TestSerial_SimpleNestedStruct orig;
            orig.vs.push_back({ 42, 43 });
            orig.vs.push_back({ 1, 2 });
            check_save_and_load_round_trip(orig);
        }
    }

    SUBCASE("Component") {
        SUBCASE("Unique pointer") {
            auto orig = lm::comp::create<lm::Component>("testserial_simple", "", {
                { "v1", 42 },
                { "v2", 32 }
            });
            check_save_and_load_round_trip_compare_loaded(orig);
        }

        SUBCASE("Vector of unique pointer") {
            auto v1 = lm::comp::create<lm::Component>("testserial_simple", "", {
                { "v1", 42 },
                { "v2", 32 }
            });
            auto v2 = lm::comp::create<lm::Component>("testserial_simple", "", {
                { "v1", 1 },
                { "v2", 2 }
            });
            std::vector<lm::Component::Ptr<lm::Component>> orig;
            orig.push_back(std::move(v1));
            orig.push_back(std::move(v2));
            check_save_and_load_round_trip_compare_loaded(orig);
        }

        SUBCASE("Nested component") {
            auto orig = lm::comp::create<lm::Component>("testserial_nested", "");
            check_save_and_load_round_trip_compare_loaded(orig);
        }

        SUBCASE("Weak reference to another component instance") {
            // Register TestSerial_Container as root component for this specific test
            TestSerial_Container container;
            lm::comp::detail::Access::loc(&container) = "$";
            container.add("p1", "testserial_simple", { { "v1", 1 }, { "v2", 2 } });
            lm::comp::detail::register_root_comp(&container);
            
            // Check serialization of Component*
            auto* orig = lm::comp::get<lm::Component>("$.p1");
            CHECK(orig);
                        
            // Round-trip test
            check_save_and_load_round_trip_compare_loaded(orig);

            // Check values
            std::stringstream ss;
            lm::serial::save(ss, orig);
            lm::Component* loaded;
            lm::serial::load(ss, loaded);
            
            auto* p1 = dynamic_cast<TestSerial_Simple*>(loaded);
            CHECK(p1->v1 == 1);
            CHECK(p1->v2 == 2);
        }

        SUBCASE("Nested container") {
            /*
                This will test more advanced case. Here we try to serialize a component
                containing two nested container components where one container only
                contains references to the instances inside the other container.  
            */

            // Root component
            TestSerial_Root root;
            root.p = lm::comp::create<TestSerial_Container>("testserial_container", "$.p");
            auto* c = dynamic_cast<TestSerial_Container*>(root.p.get());
            
            // Add nested containers
            c->add("instances", "testserial_container", {});
            c->add("references", "testserial_container", {});

            // Add instances to `instances`
            auto* instances = lm::comp::get<TestSerial_Container>("$.p.instances");
            CHECK(instances);
            instances->add("p1", "testserial_simple", { {"v1", 1}, {"v2", 2} });
            instances->add("p2", "testserial_simple", { {"v1", 3}, {"v2", 4} });
            
            // Add references to `references`
            auto* refs = lm::comp::get<TestSerial_Container>("$.p.references");
            CHECK(refs);
            refs->add("r1", "testserial_ref", { {"ref", "$.p.instances.p1"} });
            refs->add("r2", "testserial_ref", { {"ref", "$.p.instances.p2"} });
            
            // Save current state
            auto s1 = root.saveState();
 
            // Dirty hack.
            // We repeat load/save twice because cereal changes
            // byte order of some containers in a cycle.
            std::string s2 = s1;
            for (int i = 0; i < 2; i++) {
                // Clear root
                root.clear();

                // Load root from the saved state
                root.loadState(s2);

                // Save again
                s2 = root.saveState();
            }

            // Compare s1 and s2
            CHECK(s1 == s2);
        }

        SUBCASE("Assets") {
            // Round-trip tests for various assets
            SUBCASE("Film") {
                check_save_and_load_round_trip_compare_loaded(
                    lm::comp::create<lm::Component>("film::bitmap", {}, {
                        {"w", 200}, {"h", 100}
                    })
                );
            }

            SUBCASE("Mesh") {
                check_save_and_load_round_trip_compare_loaded(
                    lm::comp::create<lm::Component>("mesh::raw", {}, {
                        {"ps", {-1,-1,-1,1,-1,-1,1,1,-1,-1,1,-1}},
                        {"ns", {0,0,1}},
                        {"ts", {0,0,1,0,1,1,0,1}},
                        {"fs", {
                            {"p", {0,1,2,0,2,3}},
                            {"n", {0,0,0,0,0,0}},
                            {"t", {0,1,2,0,2,3}}
                        }}
                    })
                );
            }

            SUBCASE("Material") {
                check_save_and_load_round_trip_compare_loaded(
                    lm::comp::create<lm::Component>("material::diffuse", {}, {
                        {"Kd", {.5,1,.2}}
                    })
                );
                check_save_and_load_round_trip_compare_loaded(
                    lm::comp::create<lm::Component>("material::glass", {}, {
                        {"Ni", .5}
                    })
                );
                check_save_and_load_round_trip_compare_loaded(
                    lm::comp::create<lm::Component>("material::glossy", {}, {
                        {"Ks", {1,0,.5}},
                        {"ax", .5},
                        {"ay", .2}
                    })
                );
                check_save_and_load_round_trip_compare_loaded(
                    lm::comp::create<lm::Component>("material::mask", {}, {})
                );
                check_save_and_load_round_trip_compare_loaded(
                    lm::comp::create<lm::Component>("material::mirror", {}, {})
                );
            }
        }
    }
}

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
