#include <gtest/gtest.h>

#include <string>

#include "dynamic_array.hpp"

// Lifetime tracker struct to verify zero leaks and proper destruction
struct LifetimeTracker {
    static inline int live_instances = 0;
    static inline int copy_count = 0;
    static inline int move_count = 0;

    int value;

    explicit LifetimeTracker(int v = 0) : value(v) { ++live_instances; }

    LifetimeTracker(const LifetimeTracker& other) : value(other.value) {
        ++live_instances;
        ++copy_count;
    }

    LifetimeTracker(LifetimeTracker&& other) noexcept : value(other.value) {
        other.value = -1;
        ++live_instances;
        ++move_count;
    }

    LifetimeTracker& operator=(const LifetimeTracker& other) {
        if (this == &other) return *this;
        value = other.value;
        ++copy_count;
        return *this;
    }

    LifetimeTracker& operator=(LifetimeTracker&& other) noexcept {
        if (this == &other) return *this;
        value = other.value;
        other.value = -1;
        ++move_count;
        return *this;
    }

    ~LifetimeTracker() { --live_instances; }

    auto operator<=>(const LifetimeTracker&) const = default;
};

class DynamicArrayLifecycleTest : public ::testing::Test {
   protected:
    void SetUp() override {
        LifetimeTracker::live_instances = 0;
        LifetimeTracker::copy_count = 0;
        LifetimeTracker::move_count = 0;
    }

    void TearDown() override {
        EXPECT_EQ(LifetimeTracker::live_instances, 0) << "Memory/object leak detected!";
    }
};

TEST_F(DynamicArrayLifecycleTest, DefaultConstructorDoesNotAllocate) {
    dynamicArray<LifetimeTracker> arr;
    EXPECT_TRUE(arr.empty());
    EXPECT_EQ(arr.size(), 0u);
    EXPECT_EQ(arr.capacity(), 0u);
    EXPECT_EQ(arr.data(), nullptr);
    EXPECT_EQ(LifetimeTracker::live_instances, 0);
}

TEST_F(DynamicArrayLifecycleTest, SizedConstructorValueInitializes) {
    {
        dynamicArray<LifetimeTracker> arr(5, LifetimeTracker(100));
        EXPECT_EQ(arr.size(), 5u);
        EXPECT_GE(arr.capacity(), 5u);
        EXPECT_EQ(LifetimeTracker::live_instances, 5);
        for (std::size_t i = 0; i < arr.size(); ++i) {
            EXPECT_EQ(arr[i].value, 100);
        }
    }
    EXPECT_EQ(LifetimeTracker::live_instances, 0);
}

TEST_F(DynamicArrayLifecycleTest, InitializerListConstructor) {
    dynamicArray<int> arr{10, 20, 30, 40, 50};
    EXPECT_EQ(arr.size(), 5u);
    EXPECT_EQ(arr[0], 10);
    EXPECT_EQ(arr[2], 30);
    EXPECT_EQ(arr[4], 50);
}

TEST_F(DynamicArrayLifecycleTest, PushBackAndGrowth) {
    dynamicArray<int> arr;
    for (int i = 0; i < 64; ++i) {
        arr.push_back(i);
        EXPECT_EQ(arr.size(), static_cast<std::size_t>(i + 1));
        EXPECT_GE(arr.capacity(), arr.size());
    }
    for (int i = 0; i < 64; ++i) {
        EXPECT_EQ(arr[i], i);
    }
}

TEST_F(DynamicArrayLifecycleTest, EmplaceBackForwardsArguments) {
    dynamicArray<std::pair<int, std::string>> arr;
    arr.emplace_back(1, "one");
    arr.emplace_back(2, "two");
    EXPECT_EQ(arr.size(), 2u);
    EXPECT_EQ(arr[0].first, 1);
    EXPECT_EQ(arr[0].second, "one");
    EXPECT_EQ(arr[1].first, 2);
    EXPECT_EQ(arr[1].second, "two");
}

TEST_F(DynamicArrayLifecycleTest, SelfReferentialPushBackDuringReallocation) {
    dynamicArray<int> arr;
    arr.push_back(99);  // size=1, cap=1
    // Reallocation trigger: push_back must read arr[0] safely before relocating buffer
    arr.push_back(arr[0]);
    EXPECT_EQ(arr.size(), 2u);
    EXPECT_EQ(arr[0], 99);
    EXPECT_EQ(arr[1], 99);

    // Another reallocation
    arr.push_back(arr[0]);
    EXPECT_EQ(arr.size(), 3u);
    EXPECT_EQ(arr[2], 99);
}

TEST_F(DynamicArrayLifecycleTest, CopyConstructorAndAssignment) {
    dynamicArray<LifetimeTracker> original;
    original.emplace_back(1);
    original.emplace_back(2);
    original.emplace_back(3);
    EXPECT_EQ(LifetimeTracker::live_instances, 3);

    // Copy construction
    {
        dynamicArray<LifetimeTracker> copy_arr(original);
        EXPECT_EQ(copy_arr.size(), 3u);
        EXPECT_EQ(copy_arr[1].value, 2);
        EXPECT_EQ(LifetimeTracker::live_instances, 6);
    }
    EXPECT_EQ(LifetimeTracker::live_instances, 3);

    // Copy assignment
    dynamicArray<LifetimeTracker> assigned;
    assigned = original;
    EXPECT_EQ(assigned.size(), 3u);
    EXPECT_EQ(assigned[2].value, 3);
    EXPECT_EQ(LifetimeTracker::live_instances, 6);
}

TEST_F(DynamicArrayLifecycleTest, MoveConstructorAndAssignment) {
    dynamicArray<LifetimeTracker> source;
    source.emplace_back(10);
    source.emplace_back(20);
    EXPECT_EQ(LifetimeTracker::live_instances, 2);

    // Move construct
    dynamicArray<LifetimeTracker> moved_target(std::move(source));
    EXPECT_EQ(moved_target.size(), 2u);
    EXPECT_TRUE(source.empty());
    EXPECT_EQ(source.data(), nullptr);
    EXPECT_EQ(LifetimeTracker::live_instances, 2);

    // Move assign
    dynamicArray<LifetimeTracker> assigned;
    assigned = std::move(moved_target);
    EXPECT_EQ(assigned.size(), 2u);
    EXPECT_TRUE(moved_target.empty());
    EXPECT_EQ(LifetimeTracker::live_instances, 2);
}

TEST_F(DynamicArrayLifecycleTest, CapacityOperations) {
    dynamicArray<int> arr;
    arr.reserve(100);
    EXPECT_EQ(arr.capacity(), 100u);
    EXPECT_EQ(arr.size(), 0u);

    arr.resize(10, 42);
    EXPECT_EQ(arr.size(), 10u);
    EXPECT_EQ(arr[9], 42);

    arr.pop_back();
    EXPECT_EQ(arr.size(), 9u);

    arr.shrink_to_fit();
    EXPECT_EQ(arr.capacity(), 9u);
    EXPECT_EQ(arr.size(), 9u);

    arr.clear();
    EXPECT_TRUE(arr.empty());
    EXPECT_EQ(arr.capacity(), 9u);  // Clear preserves capacity
}

TEST_F(DynamicArrayLifecycleTest, ElementAccessBoundsChecking) {
    dynamicArray<int> arr{10, 20, 30};
    EXPECT_EQ(arr[0], 10);
    EXPECT_EQ(arr.front(), 10);
    EXPECT_EQ(arr.back(), 30);
    EXPECT_EQ(arr.at(1), 20);

    EXPECT_THROW((void)arr.at(3), std::out_of_range);
    EXPECT_THROW((void)arr.at(100), std::out_of_range);
}
