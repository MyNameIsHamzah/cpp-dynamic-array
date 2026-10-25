#include <gtest/gtest.h>

#include <cstdint>

#include "arena_allocator.hpp"
#include "dynamic_array.hpp"

TEST(ArenaTest, DirectAllocationAndBumpPointer) {
    Arena arena(1024);
    EXPECT_EQ(arena.capacity(), 1024u);
    EXPECT_EQ(arena.bytes_allocated(), 0u);
    EXPECT_EQ(arena.bytes_remaining(), 1024u);

    void* p1 = arena.allocate(100, 8);
    EXPECT_NE(p1, nullptr);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(p1) % 8, 0u);
    EXPECT_GE(arena.bytes_allocated(), 100u);

    void* p2 = arena.allocate(200, 16);
    EXPECT_NE(p2, nullptr);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(p2) % 16, 0u);

    // Reset reclaims memory
    arena.reset();
    EXPECT_EQ(arena.bytes_allocated(), 0u);
    EXPECT_EQ(arena.bytes_remaining(), 1024u);
}

TEST(ArenaTest, ThrowsBadAllocWhenExhausted) {
    Arena arena(128);
    EXPECT_NO_THROW((void)arena.allocate(100));
    EXPECT_THROW((void)arena.allocate(50), std::bad_alloc);
}

TEST(ArenaTest, StackArenaAllocation) {
    StackArena<512> stack_arena;
    EXPECT_EQ(stack_arena.capacity(), 512u);
    void* ptr = stack_arena.allocate(64, 8);
    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(ptr) % 8, 0u);
}

TEST(DynamicArrayArenaIntegration, BasicPushBackAndGrowth) {
    Arena arena(4096);
    ArenaAllocator<int> alloc(arena);

    dynamicArray<int, ArenaAllocator<int>> arr(alloc);
    EXPECT_TRUE(arr.empty());

    for (int i = 0; i < 50; ++i) {
        arr.push_back(i);
    }

    EXPECT_EQ(arr.size(), 50u);
    for (int i = 0; i < 50; ++i) {
        EXPECT_EQ(arr[i], i);
    }

    // Verify memory was taken from the arena
    EXPECT_GT(arena.bytes_allocated(), 50 * sizeof(int));
}

TEST(DynamicArrayArenaIntegration, OverAlignedTypesAreAligned) {
    struct alignas(64) CacheLineData {
        int x{0};
        int y{0};
    };

    Arena arena(4096);
    ArenaAllocator<CacheLineData> alloc(arena);

    dynamicArray<CacheLineData, ArenaAllocator<CacheLineData>> arr(alloc);
    for (int i = 0; i < 10; ++i) {
        arr.push_back(CacheLineData{i, i * 2});
    }

    for (std::size_t i = 0; i < arr.size(); ++i) {
        EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&arr[i]) % 64, 0u)
            << "Element " << i << " failed 64-byte cacheline alignment!";
        EXPECT_EQ(arr[i].x, static_cast<int>(i));
    }
}

TEST(DynamicArrayArenaIntegration, ArenaResetCycle) {
    Arena arena(2048);
    ArenaAllocator<int> alloc(arena);

    {
        dynamicArray<int, ArenaAllocator<int>> arr(alloc);
        for (int i = 0; i < 20; ++i) {
            arr.push_back(i);
        }
        EXPECT_EQ(arr.size(), 20u);
    }  // arr destructed, but arena retains memory

    EXPECT_GT(arena.bytes_allocated(), 0u);

    // Instant arena reset: typical in trading tick cycles
    arena.reset();
    EXPECT_EQ(arena.bytes_allocated(), 0u);

    {
        dynamicArray<int, ArenaAllocator<int>> arr2(alloc);
        for (int i = 0; i < 20; ++i) {
            arr2.push_back(i * 10);
        }
        EXPECT_EQ(arr2.size(), 20u);
        EXPECT_EQ(arr2[0], 0);
        EXPECT_EQ(arr2[19], 190);
    }
}
