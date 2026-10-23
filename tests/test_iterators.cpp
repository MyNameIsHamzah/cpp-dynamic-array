#include <gtest/gtest.h>

#include <algorithm>
#include <iterator>
#include <numeric>
#include <ranges>

#include "dynamic_array.hpp"

TEST(DynamicArrayIterators, ConceptAssertions) {
    static_assert(std::random_access_iterator<dynamicArray<int>::Iterator>);
    static_assert(std::contiguous_iterator<dynamicArray<int>::Iterator>);
    static_assert(std::random_access_iterator<dynamicArray<int>::ConstIterator>);
    static_assert(std::contiguous_iterator<dynamicArray<int>::ConstIterator>);
}

TEST(DynamicArrayIterators, ForwardAndBackwardTraversal) {
    dynamicArray<int> arr{1, 2, 3, 4, 5};

    int val = 1;
    for (auto it = arr.begin(); it != arr.end(); ++it) {
        EXPECT_EQ(*it, val++);
    }

    auto it = arr.end();
    --it;
    EXPECT_EQ(*it, 5);
    --it;
    EXPECT_EQ(*it, 4);
}

TEST(DynamicArrayIterators, RandomAccessArithmetic) {
    dynamicArray<int> arr{10, 20, 30, 40, 50};

    auto it = arr.begin();
    EXPECT_EQ(*(it + 3), 40);
    EXPECT_EQ(*(3 + it), 40);

    it += 4;
    EXPECT_EQ(*it, 50);

    it -= 2;
    EXPECT_EQ(*it, 30);
    EXPECT_EQ(*(it - 1), 20);

    // Subscript on iterator
    EXPECT_EQ(arr.begin()[3], 40);

    // Difference
    EXPECT_EQ(arr.end() - arr.begin(), 5);
}

TEST(DynamicArrayIterators, RelationalComparisonsAndConstness) {
    dynamicArray<int> arr{1, 2, 3, 4};
    auto it1 = arr.begin();
    auto it2 = arr.begin() + 2;

    EXPECT_TRUE(it1 < it2);
    EXPECT_TRUE(it2 > it1);
    EXPECT_TRUE(it1 <= it1);
    EXPECT_TRUE(it2 >= it1);
    EXPECT_TRUE(it1 != it2);

    // Cross-const comparison
    const auto& const_arr = arr;
    auto cit = const_arr.cbegin();
    EXPECT_TRUE(it1 == cit);
    EXPECT_TRUE(cit < it2);
    EXPECT_EQ(it2 - cit, 2);
}

TEST(DynamicArrayIterators, ContiguityVerification) {
    dynamicArray<int> arr{10, 20, 30};
    for (std::size_t i = 0; i < arr.size(); ++i) {
        EXPECT_EQ(std::to_address(arr.begin() + i), arr.data() + i);
    }
}

TEST(DynamicArrayIterators, StandardAlgorithmsCompatibility) {
    dynamicArray<int> arr{5, 2, 8, 1, 9, 3};

    std::sort(arr.begin(), arr.end());
    EXPECT_TRUE(std::is_sorted(arr.begin(), arr.end()));
    EXPECT_TRUE(std::binary_search(arr.begin(), arr.end(), 8));
    EXPECT_FALSE(std::binary_search(arr.begin(), arr.end(), 7));

    std::reverse(arr.begin(), arr.end());
    EXPECT_EQ(arr[0], 9);
    EXPECT_EQ(arr[5], 1);
}

TEST(DynamicArrayIterators, RangesAlgorithmsCompatibility) {
    dynamicArray<int> arr{4, 1, 3, 2};

    std::ranges::sort(arr);
    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[1], 2);
    EXPECT_EQ(arr[2], 3);
    EXPECT_EQ(arr[3], 4);

    std::ranges::reverse(arr);
    EXPECT_EQ(arr[0], 4);
    EXPECT_EQ(arr[3], 1);

    auto found = std::ranges::find(arr, 3);
    EXPECT_NE(found, arr.end());
    EXPECT_EQ(found - arr.begin(), 1);
}
