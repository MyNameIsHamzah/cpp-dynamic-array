#include <gtest/gtest.h>

#include <stdexcept>

#include "dynamic_array.hpp"

struct ThrowingElement {
    static inline int live_instances = 0;
    static inline bool throw_on_copy = false;
    static inline bool throw_on_construct = false;

    int value;

    explicit ThrowingElement(int v = 0) : value(v) {
        if (throw_on_construct) {
            throw std::runtime_error("Simulated constructor failure");
        }
        ++live_instances;
    }

    ThrowingElement(const ThrowingElement& other) : value(other.value) {
        if (throw_on_copy) {
            throw std::runtime_error("Simulated copy failure");
        }
        ++live_instances;
    }

    // Deliberately NOT noexcept so relocation falls back to copy to preserve strong exception
    // guarantee
    ThrowingElement(ThrowingElement&& other) {
        if (throw_on_copy) {
            throw std::runtime_error("Simulated move failure");
        }
        value = other.value;
        other.value = -1;
        ++live_instances;
    }

    ThrowingElement& operator=(const ThrowingElement& other) {
        if (this == &other) return *this;
        if (throw_on_copy) {
            throw std::runtime_error("Simulated copy assignment failure");
        }
        value = other.value;
        return *this;
    }

    ThrowingElement& operator=(ThrowingElement&& other) noexcept {
        if (this == &other) return *this;
        value = other.value;
        other.value = -1;
        return *this;
    }

    ~ThrowingElement() { --live_instances; }
};

class ExceptionSafetyTest : public ::testing::Test {
   protected:
    void SetUp() override {
        ThrowingElement::live_instances = 0;
        ThrowingElement::throw_on_copy = false;
        ThrowingElement::throw_on_construct = false;
    }

    void TearDown() override {
        ThrowingElement::throw_on_copy = false;
        ThrowingElement::throw_on_construct = false;
        EXPECT_EQ(ThrowingElement::live_instances, 0) << "Exception caused leak of live instances!";
    }
};

TEST_F(ExceptionSafetyTest, StrongGuaranteeOnReserve) {
    dynamicArray<ThrowingElement> arr;
    arr.emplace_back(100);
    arr.emplace_back(200);
    EXPECT_EQ(ThrowingElement::live_instances, 2);

    // Simulate failure during relocation
    ThrowingElement::throw_on_copy = true;

    EXPECT_THROW(arr.reserve(50), std::runtime_error);

    // Container state must be completely preserved!
    EXPECT_EQ(arr.size(), 2u);
    EXPECT_EQ(arr.capacity(), 2u);
    EXPECT_EQ(arr[0].value, 100);
    EXPECT_EQ(arr[1].value, 200);
    EXPECT_EQ(ThrowingElement::live_instances, 2);
}

TEST_F(ExceptionSafetyTest, StrongGuaranteeOnEmplaceBackConstructorThrow) {
    dynamicArray<ThrowingElement> arr;
    arr.emplace_back(1);
    arr.emplace_back(2);
    EXPECT_EQ(ThrowingElement::live_instances, 2);

    // Cause constructor of newly added element to throw
    ThrowingElement::throw_on_construct = true;

    EXPECT_THROW(arr.emplace_back(3), std::runtime_error);

    // Original elements must remain intact with no dangling allocations
    EXPECT_EQ(arr.size(), 2u);
    EXPECT_EQ(arr[0].value, 1);
    EXPECT_EQ(arr[1].value, 2);
    EXPECT_EQ(ThrowingElement::live_instances, 2);
}

TEST_F(ExceptionSafetyTest, StrongGuaranteeOnCopyAssignment) {
    dynamicArray<ThrowingElement> src;
    src.emplace_back(1);
    src.emplace_back(2);

    dynamicArray<ThrowingElement> dst;
    dst.emplace_back(10);
    dst.emplace_back(20);
    dst.emplace_back(30);

    // Simulate throw while copying into dst
    ThrowingElement::throw_on_copy = true;

    EXPECT_THROW(dst = src, std::runtime_error);

    // Destination must remain intact or properly destructible with no leaks
    EXPECT_EQ(ThrowingElement::live_instances, 5);  // 2 in src + 3 in dst
}
