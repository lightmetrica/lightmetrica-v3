/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include "test_common.h"
#include <lm/serial.h>

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
void checkSaveAndLoadRoundTrip(T&& orig) {
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

    // Serialize again with different stream
    std::stringstream ss2;
    lm::serial::save(ss2, loaded);

    // Compare two streams
    const auto s1 = ss1.str();
    const auto s2 = ss2.str();
    CHECK(s1 == s2);
}

// ----------------------------------------------------------------------------

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

struct TestSerial_SimpleComponent : public lm::Component {
    int v1;
    int v2;
    virtual bool construct(const lm::Json& prop) {
        v1 = prop["v1"];
        v2 = prop["v2"];
        return true;
    }
    virtual void load(lm::InputArchive& ar) {
        LM_UNUSED(ar);
    }
    virtual void save(lm::OutputArchive& ar) {
        LM_UNUSED(ar);
    }
};

LM_COMP_REG_IMPL(TestSerial_SimpleComponent, "testserial_simplecomponent");

// ----------------------------------------------------------------------------

TEST_CASE("Serialization") {
    lm::log::ScopedInit init;
    
    SUBCASE("Primitive types") {
        checkSaveAndLoadRoundTrip<bool>(true);
        checkSaveAndLoadRoundTrip<int>(42);
        checkSaveAndLoadRoundTrip<double>(42.0);
        checkSaveAndLoadRoundTrip<float>(42.f);
        checkSaveAndLoadRoundTrip<std::string>("hai domo");
    }

    SUBCASE("STL containers") {
        checkSaveAndLoadRoundTrip<std::vector<int>>({ 1,2,3,4,5 });
        checkSaveAndLoadRoundTrip<std::unordered_map<std::string, int>>({
            { "A", 1 },
            { "B", 2 },
            { "C", 3 }
        });
    }

    SUBCASE("Simple struct") {
        SUBCASE("Simple") {
            TestSerial_SimpleStruct orig{ 42, 43 };
            checkSaveAndLoadRoundTrip(orig);
        }
        SUBCASE("Nested") {
            TestSerial_SimpleNestedStruct orig;
            orig.vs.push_back({ 42, 43 });
            orig.vs.push_back({ 1, 2 });
            checkSaveAndLoadRoundTrip(orig);
        }
    }

    SUBCASE("Component") {
        SUBCASE("Unique pointer") {
            auto orig = lm::comp::create<lm::Component>("testserial_simplecomponent", {
                { "v1", 42 },
                { "v2", 32 }
            });
            std::stringstream ss;
            lm::serial::save(ss, orig);
        }
    }
}

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
