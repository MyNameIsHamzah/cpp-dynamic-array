#include <gtest/gtest.h>

#include <algorithm>
#include <ranges>

#include "dynamic_array.hpp"

TEST(DynamicArrayBool, ConceptAssertions) {
    static_assert(std::random_access_iterator<dynamicArray<bool>::Iterator>);
    static_assert(std::random_access_iterator<dynamicArray<bool>::ConstIterator>);
}

TEST(DynamicArrayBool, MultiByteSpanningInitialization) {
    dynamicArray<bool> barr{
        true, false, true,  true, false, false, true, false,  // byte 0
        true, true,  false, true, false, true,  true, false,  // byte 1
        true, false, true                                     // byte 2 (19 elements)
    };

    EXPECT_EQ(barr.size(), 19u);
    EXPECT_GE(barr.capacity(), 24u);  // At least 3 bytes worth of bits

    EXPECT_TRUE(barr[0]);
    EXPECT_FALSE(barr[1]);
    EXPECT_TRUE(barr[8]);
    EXPECT_FALSE(barr[10]);
    EXPECT_TRUE(barr[18]);
}

TEST(DynamicArrayBool, ProxyMutationAndFlip) {
    dynamicArray<bool> barr{true, false};

    barr[0] = false;
    EXPECT_FALSE(barr[0]);

    barr[1] = true;
    EXPECT_TRUE(barr[1]);

    // Flip proxy
    auto proxy = barr[0];
    proxy.flip();
    EXPECT_TRUE(barr[0]);
}

TEST(DynamicArrayBool, RandomAccessIteratorArithmetic) {
    dynamicArray<bool> barr{
        true,  false, true,  false, true,  false, true,  false,  // byte 0
        false, true,  false, true,  false, true,  false, true    // byte 1 (16 elements)
    };

    auto it = barr.begin();
    EXPECT_TRUE(*it);
    EXPECT_FALSE(*(it + 1));
    EXPECT_FALSE(*(it + 8));
    EXPECT_TRUE(*(it + 9));

    // Pointer arithmetic advancement
    it += 9;
    EXPECT_TRUE(*it);

    it -= 8;
    EXPECT_FALSE(*it);

    // Subscript indexing on iterator
    EXPECT_TRUE(barr.begin()[0]);
    EXPECT_FALSE(barr.begin()[1]);
    EXPECT_TRUE(barr.begin()[9]);

    // Difference across byte boundaries
    EXPECT_EQ(barr.end() - barr.begin(), 16);
}

TEST(DynamicArrayBool, RangesAlgorithmsAndIterSwap) {
    dynamicArray<bool> barr{true, false, false, true, true};

    EXPECT_EQ(std::count(barr.begin(), barr.end(), true), 3);
    EXPECT_EQ(std::count(barr.begin(), barr.end(), false), 2);

    auto found = std::find(barr.begin(), barr.end(), false);
    EXPECT_NE(found, barr.end());
    EXPECT_EQ(found - barr.begin(), 1);

    // In-place bit reverse using std::ranges::reverse
    dynamicArray<bool> bits{true, false, true, true, false};
    std::ranges::reverse(bits);
    EXPECT_FALSE(bits[0]);
    EXPECT_TRUE(bits[1]);
    EXPECT_TRUE(bits[2]);
    EXPECT_FALSE(bits[3]);
    EXPECT_TRUE(bits[4]);
}

TEST(DynamicArrayBool, PushBackGrowth) {
    dynamicArray<bool> barr;
    for (int i = 0; i < 33; ++i) {
        barr.push_back((i % 2) == 0);
        EXPECT_EQ(barr.size(), static_cast<std::size_t>(i + 1));
    }

    for (int i = 0; i < 33; ++i) {
        EXPECT_EQ(barr[i], (i % 2) == 0);
    }
}
