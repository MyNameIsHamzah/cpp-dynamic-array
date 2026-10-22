#include "dynamic_array.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

// Test utility struct to verify RAII, no leaks, and exception safety
struct Tracked {
    static inline int live_count = 0;
    static inline int throw_after = -1;
    static inline int copy_count = 0;

    int value;

    Tracked(int v = 0) : value(v) { ++live_count; }

    Tracked(const Tracked& other) : value(other.value) {
        if (throw_after == 0) {
            throw std::runtime_error("Simulated copy exception");
        }
        if (throw_after > 0) {
            --throw_after;
        }
        ++live_count;
        ++copy_count;
    }

    Tracked(Tracked&& other) noexcept : value(other.value) {
        other.value = -1;
        ++live_count;
    }

    Tracked& operator=(const Tracked& other) {
        if (this == &other) return *this;
        value = other.value;
        return *this;
    }

    Tracked& operator=(Tracked&& other) noexcept {
        if (this == &other) return *this;
        value = other.value;
        other.value = -1;
        return *this;
    }

    ~Tracked() { --live_count; }

    auto operator<=>(const Tracked&) const = default;
};

void test_constructors_and_raii() {
    std::cout << "[Test] Constructors & RAII... ";
    {
        dynamicArray<Tracked> arr;
        assert(arr.empty());
        assert(arr.size() == 0);
        assert(arr.capacity() == 0);
        assert(Tracked::live_count == 0);
    }
    assert(Tracked::live_count == 0);

    {
        dynamicArray<Tracked> arr(5, Tracked(42));
        assert(arr.size() == 5);
        assert(arr.capacity() == 5);
        assert(Tracked::live_count == 5);
        for (std::size_t i = 0; i < arr.size(); ++i) {
            assert(arr[i].value == 42);
        }
    }
    assert(Tracked::live_count == 0);  // Verified clean destruction

    {
        dynamicArray<int> arr{10, 20, 30, 40};
        assert(arr.size() == 4);
        assert(arr[0] == 10);
        assert(arr[3] == 40);
    }

    std::cout << "PASSED\n";
}

void test_push_back_and_growth() {
    std::cout << "[Test] Push_back & Growth... ";
    {
        dynamicArray<int> arr;
        for (int i = 0; i < 100; ++i) {
            arr.push_back(i);
            assert(arr.size() == static_cast<std::size_t>(i + 1));
            assert(arr.capacity() >= arr.size());
        }
        for (int i = 0; i < 100; ++i) {
            assert(arr[i] == i);
        }
    }
    std::cout << "PASSED\n";
}

void test_self_referential_push_back() {
    std::cout << "[Test] Self-referencing push_back during reallocation... ";
    {
        dynamicArray<int> arr;
        arr.push_back(42);  // size = 1, capacity = 1
        // Next push triggers reallocation: arr.push_back(arr[0]) MUST NOT read dangling memory
        arr.push_back(arr[0]);
        assert(arr.size() == 2);
        assert(arr[0] == 42);
        assert(arr[1] == 42);

        // Another reallocation trigger
        arr.push_back(arr[0]);
        assert(arr.size() == 3);
        assert(arr[2] == 42);
    }
    std::cout << "PASSED\n";
}

void test_copy_and_move_semantics() {
    std::cout << "[Test] Copy & Move semantics... ";
    {
        dynamicArray<Tracked> original;
        original.emplace_back(1);
        original.emplace_back(2);
        original.emplace_back(3);
        assert(Tracked::live_count == 3);

        // Copy constructor
        dynamicArray<Tracked> copy_arr(original);
        assert(copy_arr.size() == 3);
        assert(copy_arr[1].value == 2);
        assert(Tracked::live_count == 6);

        // Copy assignment
        dynamicArray<Tracked> copy_assign;
        copy_assign = original;
        assert(copy_assign.size() == 3);
        assert(Tracked::live_count == 9);

        // Move constructor
        dynamicArray<Tracked> moved_arr(std::move(copy_arr));
        assert(moved_arr.size() == 3);
        assert(copy_arr.empty());
        assert(copy_arr.data() == nullptr);
        assert(Tracked::live_count == 9);

        // Move assignment
        copy_assign = std::move(moved_arr);
        assert(copy_assign.size() == 3);
        assert(moved_arr.empty());
        assert(Tracked::live_count == 6);
    }
    assert(Tracked::live_count == 0);
    std::cout << "PASSED\n";
}

struct ThrowingType {
    static inline int live_count = 0;
    static inline bool should_throw = false;
    int value;

    ThrowingType(int v = 0) : value(v) { ++live_count; }

    ThrowingType(const ThrowingType& other) : value(other.value) {
        if (should_throw) {
            throw std::runtime_error("Simulated copy exception");
        }
        ++live_count;
    }

    // Deliberately NOT noexcept so relocation falls back to copy for strong exception guarantee
    ThrowingType(ThrowingType&& other) {
        if (should_throw) {
            throw std::runtime_error("Simulated move exception");
        }
        value = other.value;
        other.value = -1;
        ++live_count;
    }

    ~ThrowingType() { --live_count; }
};

void test_strong_exception_safety() {
    std::cout << "[Test] Strong exception safety during reallocation... ";
    {
        dynamicArray<ThrowingType> arr;
        arr.emplace_back(10);
        arr.emplace_back(20);
        assert(ThrowingType::live_count == 2);

        // Enable simulated failure
        ThrowingType::should_throw = true;
        bool caught = false;
        try {
            // reserve requires reallocation and copying since move is not noexcept
            arr.reserve(10);
        } catch (const std::runtime_error&) {
            caught = true;
        }
        assert(caught);
        // Container invariants must be intact!
        assert(arr.size() == 2);
        assert(arr.capacity() == 2);
        assert(arr[0].value == 10);
        assert(arr[1].value == 20);
        assert(ThrowingType::live_count == 2);

        ThrowingType::should_throw = false;  // reset
    }
    assert(ThrowingType::live_count == 0);  // No memory or object leaks!
    std::cout << "PASSED\n";
}

void test_iterators_and_algorithms() {
    std::cout << "[Test] Bidirectional Iterators & Traversal... ";
    dynamicArray<int> arr{1, 2, 3, 4, 5};

    // Forward iteration
    int expected = 1;
    for (auto it = arr.begin(); it != arr.end(); ++it) {
        assert(*it == expected);
        ++expected;
    }

    // Backward iteration (testing operator-- fix)
    auto it = arr.end();
    --it;
    assert(*it == 5);
    --it;
    assert(*it == 4);

    // Const iteration
    const auto& const_arr = arr;
    auto cit = const_arr.cbegin();
    assert(*cit == 1);
    assert(cit == const_arr.begin());

    std::cout << "PASSED\n";
}

void test_capacity_operations() {
    std::cout << "[Test] Reserve, shrink_to_fit, resize, clear, pop_back... ";
    dynamicArray<int> arr;
    arr.reserve(50);
    assert(arr.capacity() == 50);
    assert(arr.size() == 0);

    arr.resize(5, 7);
    assert(arr.size() == 5);
    assert(arr[4] == 7);

    arr.pop_back();
    assert(arr.size() == 4);

    arr.shrink_to_fit();
    assert(arr.capacity() == 4);
    assert(arr.size() == 4);

    arr.clear();
    assert(arr.empty());
    assert(arr.capacity() == 4);  // clear preserves capacity

    std::cout << "PASSED\n";
}

void test_bool_specialization_smoke() {
    std::cout << "[Test] dynamicArray<bool> basic verification... ";
    dynamicArray<bool> barr{true, false, true, true};
    assert(barr.size() == 4);
    assert(barr[0] == true);
    assert(barr[1] == false);
    assert(barr[2] == true);
    assert(barr[3] == true);

    barr[1] = true;
    assert(barr[1] == true);

    barr.push_back(false);
    assert(barr.size() == 5);
    assert(barr[4] == false);

    std::cout << "PASSED\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  dynamicArray Milestone 1 Verification \n";
    std::cout << "========================================\n";

    test_constructors_and_raii();
    test_push_back_and_growth();
    test_self_referential_push_back();
    test_copy_and_move_semantics();
    test_strong_exception_safety();
    test_iterators_and_algorithms();
    test_capacity_operations();
    test_bool_specialization_smoke();

    std::cout << "\nALL MILESTONE 1 CHECKS PASSED PERFECTLY!\n";
    return 0;
}