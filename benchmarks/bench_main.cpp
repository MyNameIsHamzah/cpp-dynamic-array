#include "arena_allocator.hpp"
#include "dynamic_array.hpp"
#include <benchmark/benchmark.h>
#include <algorithm>
#include <cstdint>
#include <random>
#include <vector>

// ============================================================================
// SUITE 1: Container Throughput (dynamicArray<int> vs std::vector<int>)
// ============================================================================

static void BM_DynamicArray_PushBack_Geometric(benchmark::State& state) {
    const auto n = state.range(0);
    for (auto _ : state) {
        dynamicArray<int> arr;
        for (int i = 0; i < n; ++i) {
            arr.push_back(i);
        }
        benchmark::DoNotOptimize(arr.data());
    }
    state.SetComplexityN(n);
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_DynamicArray_PushBack_Geometric)->RangeMultiplier(4)->Range(64, 1048576)->Complexity();

static void BM_StdVector_PushBack_Geometric(benchmark::State& state) {
    const auto n = state.range(0);
    for (auto _ : state) {
        std::vector<int> vec;
        for (int i = 0; i < n; ++i) {
            vec.push_back(i);
        }
        benchmark::DoNotOptimize(vec.data());
    }
    state.SetComplexityN(n);
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_StdVector_PushBack_Geometric)->RangeMultiplier(4)->Range(64, 1048576)->Complexity();

static void BM_DynamicArray_PushBack_Reserved(benchmark::State& state) {
    const auto n = state.range(0);
    for (auto _ : state) {
        dynamicArray<int> arr;
        arr.reserve(n);
        for (int i = 0; i < n; ++i) {
            arr.push_back(i);
        }
        benchmark::DoNotOptimize(arr.data());
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_DynamicArray_PushBack_Reserved)->RangeMultiplier(4)->Range(64, 1048576);

static void BM_StdVector_PushBack_Reserved(benchmark::State& state) {
    const auto n = state.range(0);
    for (auto _ : state) {
        std::vector<int> vec;
        vec.reserve(n);
        for (int i = 0; i < n; ++i) {
            vec.push_back(i);
        }
        benchmark::DoNotOptimize(vec.data());
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_StdVector_PushBack_Reserved)->RangeMultiplier(4)->Range(64, 1048576);

static void BM_DynamicArray_Iteration(benchmark::State& state) {
    const auto n = state.range(0);
    dynamicArray<int> arr;
    arr.reserve(n);
    for (int i = 0; i < n; ++i) arr.push_back(i);

    for (auto _ : state) {
        int64_t sum = 0;
        for (auto it = arr.begin(); it != arr.end(); ++it) {
            sum += *it;
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_DynamicArray_Iteration)->RangeMultiplier(4)->Range(1024, 1048576);

static void BM_StdVector_Iteration(benchmark::State& state) {
    const auto n = state.range(0);
    std::vector<int> vec;
    vec.reserve(n);
    for (int i = 0; i < n; ++i) vec.push_back(i);

    for (auto _ : state) {
        int64_t sum = 0;
        for (auto it = vec.begin(); it != vec.end(); ++it) {
            sum += *it;
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_StdVector_Iteration)->RangeMultiplier(4)->Range(1024, 1048576);

static void BM_DynamicArray_Sort(benchmark::State& state) {
    const auto n = state.range(0);
    std::mt19937 rng(42);
    std::vector<int> init_data(n);
    std::iota(init_data.begin(), init_data.end(), 0);
    std::shuffle(init_data.begin(), init_data.end(), rng);

    for (auto _ : state) {
        state.PauseTiming();
        dynamicArray<int> arr;
        arr.reserve(n);
        for (int v : init_data) arr.push_back(v);
        state.ResumeTiming();

        std::sort(arr.begin(), arr.end());
        benchmark::DoNotOptimize(arr.data());
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_DynamicArray_Sort)->RangeMultiplier(4)->Range(1024, 65536);

static void BM_StdVector_Sort(benchmark::State& state) {
    const auto n = state.range(0);
    std::mt19937 rng(42);
    std::vector<int> init_data(n);
    std::iota(init_data.begin(), init_data.end(), 0);
    std::shuffle(init_data.begin(), init_data.end(), rng);

    for (auto _ : state) {
        state.PauseTiming();
        std::vector<int> vec = init_data;
        state.ResumeTiming();

        std::sort(vec.begin(), vec.end());
        benchmark::DoNotOptimize(vec.data());
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_StdVector_Sort)->RangeMultiplier(4)->Range(1024, 65536);

// ============================================================================
// SUITE 2: Allocator Impact (std::allocator vs ArenaAllocator)
// Simulates an order/tick cycle: repeatedly allocate buffer, push items, destruct
// ============================================================================

static void BM_Allocator_StdHeap(benchmark::State& state) {
    const auto n = state.range(0);
    for (auto _ : state) {
        dynamicArray<int> arr;
        for (int i = 0; i < n; ++i) {
            arr.push_back(i);
        }
        benchmark::DoNotOptimize(arr.data());
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_Allocator_StdHeap)->RangeMultiplier(4)->Range(64, 4096);

static void BM_Allocator_MonotonicArena(benchmark::State& state) {
    const auto n = state.range(0);
    // Preallocate arena storage for the cycle
    const std::size_t arena_size = n * sizeof(int) * 4;
    Arena arena(arena_size);
    ArenaAllocator<int> alloc(arena);

    for (auto _ : state) {
        arena.reset(); // O(1) zero-syscall reclaim
        dynamicArray<int, ArenaAllocator<int>> arr(alloc);
        for (int i = 0; i < n; ++i) {
            arr.push_back(i);
        }
        benchmark::DoNotOptimize(arr.data());
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_Allocator_MonotonicArena)->RangeMultiplier(4)->Range(64, 4096);

// ============================================================================
// SUITE 3: The Boolean Specialization Crossover
// Measures read/scan throughput across L1/L2 cache vs main DRAM
// ============================================================================

// 1. Unspecialized 1-byte per boolean (dynamicArray<uint8_t>)
static void BM_Bool_ByteArray_Scan(benchmark::State& state) {
    const auto n = state.range(0);
    dynamicArray<uint8_t> arr;
    arr.reserve(n);
    for (int i = 0; i < n; ++i) {
        arr.push_back(static_cast<uint8_t>((i % 2) != 0));
    }

    for (auto _ : state) {
        int64_t true_count = 0;
        for (auto it = arr.begin(); it != arr.end(); ++it) {
            true_count += *it;
        }
        benchmark::DoNotOptimize(true_count);
    }
    state.SetItemsProcessed(state.iterations() * n);
    state.SetBytesProcessed(state.iterations() * n * sizeof(uint8_t));
}
BENCHMARK(BM_Bool_ByteArray_Scan)->RangeMultiplier(4)->Range(64, 4194304);

// 2. Bit-packed 1-bit per boolean (dynamicArray<bool>)
static void BM_Bool_BitPacked_Scan(benchmark::State& state) {
    const auto n = state.range(0);
    dynamicArray<bool> barr;
    for (int i = 0; i < n; ++i) {
        barr.push_back((i % 2) != 0);
    }

    for (auto _ : state) {
        int64_t true_count = 0;
        for (auto it = barr.begin(); it != barr.end(); ++it) {
            if (*it) ++true_count;
        }
        benchmark::DoNotOptimize(true_count);
    }
    state.SetItemsProcessed(state.iterations() * n);
    state.SetBytesProcessed(state.iterations() * ((n + 7) / 8));
}
BENCHMARK(BM_Bool_BitPacked_Scan)->RangeMultiplier(4)->Range(64, 4194304);

// 3. std::vector<bool> (STL bit-packed baseline)
static void BM_StdVector_Bool_Scan(benchmark::State& state) {
    const auto n = state.range(0);
    std::vector<bool> vec;
    for (int i = 0; i < n; ++i) {
        vec.push_back((i % 2) != 0);
    }

    for (auto _ : state) {
        int64_t true_count = 0;
        for (auto it = vec.begin(); it != vec.end(); ++it) {
            if (*it) ++true_count;
        }
        benchmark::DoNotOptimize(true_count);
    }
    state.SetItemsProcessed(state.iterations() * n);
    state.SetBytesProcessed(state.iterations() * ((n + 7) / 8));
}
BENCHMARK(BM_StdVector_Bool_Scan)->RangeMultiplier(4)->Range(64, 4194304);
