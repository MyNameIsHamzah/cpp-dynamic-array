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
    std::cout << "[Test] Random Access & Contiguous Iterators... ";

    // Compile-time concept assertions
    static_assert(std::random_access_iterator<dynamicArray<int>::Iterator>);
    static_assert(std::contiguous_iterator<dynamicArray<int>::Iterator>);
    static_assert(std::random_access_iterator<dynamicArray<int>::ConstIterator>);
    static_assert(std::contiguous_iterator<dynamicArray<int>::ConstIterator>);

    dynamicArray<int> arr{50, 20, 10, 40, 30};

    // Random access arithmetic
    auto it = arr.begin();
    assert(*it == 50);
    assert(*(it + 2) == 10);
    assert(*(2 + it) == 10);
    it += 3;
    assert(*it == 40);
    it -= 2;
    assert(*it == 20);
    assert(it[2] == 40);

    // Difference and comparisons
    auto it_begin = arr.begin();
    auto it_end = arr.end();
    assert(it_end - it_begin == 5);
    assert(it_begin < it_end);
    assert(it_end > it_begin);
    assert(it_begin <= it_begin);
    assert(it_begin >= it_begin);

    // Cross-const comparisons and arithmetic
    const auto& const_arr = arr;
    auto cit = const_arr.cbegin();
    assert(it_begin == cit);
    assert(cit < it_end);
    assert(it_end - cit == 5);

    // Contiguity verification (std::to_address matches buffer pointer)
    assert(std::to_address(arr.begin()) == arr.data());

    // Standard algorithms: std::sort and binary_search
    std::sort(arr.begin(), arr.end());
    assert(arr[0] == 10 && arr[1] == 20 && arr[2] == 30 && arr[3] == 40 && arr[4] == 50);
    assert(std::binary_search(arr.begin(), arr.end(), 30));

    // Ranges algorithms: std::ranges::reverse
    std::ranges::reverse(arr);
    assert(arr[0] == 50 && arr[1] == 40 && arr[2] == 30 && arr[3] == 20 && arr[4] == 10);

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

void test_bool_specialization_and_iterators() {
    std::cout << "[Test] dynamicArray<bool> Random Access Iterators & Algorithms... ";

    static_assert(std::random_access_iterator<dynamicArray<bool>::Iterator>);
    static_assert(std::random_access_iterator<dynamicArray<bool>::ConstIterator>);

    // Create 19 elements (spans 3 bytes: 8 + 8 + 3)
    dynamicArray<bool> barr{
        true, false, true,  true, false, false, true, false,  // byte 0
        true, true,  false, true, false, true,  true, false,  // byte 1
        true, false, true                                     // byte 2
    };
    assert(barr.size() == 19);

    // Subscript access
    assert(barr[0] == true);
    assert(barr[1] == false);
    assert(barr[8] == true);
    assert(barr[18] == true);

    // Random access arithmetic across byte boundaries
    auto it = barr.begin();
    assert(*it == true);
    assert(*(it + 1) == false);
    assert(*(it + 8) == true);
    assert(*(it + 18) == true);

    it += 10;  // byte 1, bit 2
    assert(*it == false);
    it -= 2;  // byte 1, bit 0
    assert(*it == true);

    // Difference and comparisons
    auto it_begin = barr.begin();
    auto it_end = barr.end();
    assert(it_end - it_begin == 19);
    assert(it_begin < it_end);
    assert(it_end > it_begin);
    assert(it_begin[8] == true);

    // Proxy mutation through iterator
    *it_begin = false;
    assert(barr[0] == false);
    *it_begin = true;
    assert(barr[0] == true);

    // Standard algorithms: std::count, std::find
    auto true_count = std::count(barr.begin(), barr.end(), true);
    assert(true_count == 11);

    auto found = std::find(barr.begin(), barr.end(), false);
    assert(found != barr.end());
    assert(found - barr.begin() == 1);

    // Reverse with std::ranges::reverse
    // Note: Legacy std::reverse dispatches on iterator_category (which is input_iterator_tag due to
    // proxy references), whereas C++20 std::ranges::reverse correctly dispatches on
    // iterator_concept (std::random_access_iterator_tag)!
    dynamicArray<bool> palindromic{true, false, true, false, true};
    std::ranges::reverse(palindromic);
    assert(palindromic[0] == true);
    assert(palindromic[1] == false);
    assert(palindromic[2] == true);
    assert(palindromic[3] == false);
    assert(palindromic[4] == true);

    std::cout << "PASSED\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  dynamicArray Milestone 2 Verification \n";
    std::cout << "========================================\n";

    test_constructors_and_raii();
    test_push_back_and_growth();
    test_self_referential_push_back();
    test_copy_and_move_semantics();
    test_strong_exception_safety();
    test_iterators_and_algorithms();
    test_capacity_operations();
    test_bool_specialization_and_iterators();

    std::cout << "\nALL MILESTONE 2 CHECKS PASSED PERFECTLY!\n";
    return 0;
}