# Benchmark Results Summary

Performance benchmarks compiled with `-O3 -DNDEBUG` comparing `dynamicArray` against `std::vector` and allocator variants.

## 1. Container PushBack Throughput (Time in ns)

| Elements (N) | dynamicArray (Geometric) | std::vector (Geometric) | Speedup vs STL | dynamicArray (Reserved) | std::vector (Reserved) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| 64 | 164.3 ns | 99.1 ns | **0.60x** | 103.5 ns | 24.5 ns |
| 256 | 505.6 ns | 198.8 ns | **0.39x** | 438.1 ns | 88.8 ns |
| 1,024 | 1,757.0 ns | 473.1 ns | **0.27x** | 1,629.5 ns | 252.2 ns |
| 4,096 | 6,908.8 ns | 1,476.5 ns | **0.21x** | 6,622.2 ns | 934.8 ns |
| 16,384 | 26,489.0 ns | 5,121.9 ns | **0.19x** | 25,486.7 ns | 3,754.7 ns |
| 65,536 | 107,272.1 ns | 19,308.6 ns | **0.18x** | 102,538.9 ns | 14,742.2 ns |
| 262,144 | 439,055.3 ns | 90,745.0 ns | **0.21x** | 422,788.7 ns | 59,472.3 ns |
| 1,048,576 | 1,703,487.7 ns | 301,619.4 ns | **0.18x** | 1,627,837.3 ns | 232,181.1 ns |

## 2. Allocator Impact: std::allocator (Heap malloc) vs ArenaAllocator (0-Syscall Bump)

| Cycle Elements | std::allocator (malloc/free) | ArenaAllocator (Bump + Reset) | Latency Reduction |
| :--- | :--- | :--- | :--- |
| 64 pushes | 163.4 ns | 112.1 ns | **1.46x faster** |
| 256 pushes | 501.2 ns | 421.4 ns | **1.19x faster** |
| 1,024 pushes | 1,753.9 ns | 1,645.3 ns | **1.07x faster** |
| 4,096 pushes | 6,768.6 ns | 6,515.4 ns | **1.04x faster** |

## 3. The Boolean Specialization Crossover (Scan Latency)

| Total Booleans | dynamicArray<uint8_t> (Byte) | dynamicArray<bool> (1-bit Packed) | std::vector<bool> (STL) | Memory Bandwidth Advantage |
| :--- | :--- | :--- | :--- | :--- |
| 64 | 4.2 ns | 38.0 ns | 33.3 ns | **0.11x** |
| 256 | 14.1 ns | 173.3 ns | 138.7 ns | **0.08x** |
| 1,024 | 52.9 ns | 691.6 ns | 540.5 ns | **0.08x** |
| 4,096 | 209.0 ns | 2,716.6 ns | 2,175.1 ns | **0.08x** |
| 16,384 | 829.2 ns | 10,868.1 ns | 8,775.2 ns | **0.08x** |
| 65,536 | 3,325.1 ns | 43,290.1 ns | 34,308.1 ns | **0.08x** |
| 262,144 | 13,350.7 ns | 173,805.9 ns | 139,391.7 ns | **0.08x** |
| 1,048,576 | 53,415.0 ns | 700,072.6 ns | 557,469.3 ns | **0.08x** |
| 4,194,304 | 212,876.3 ns | 2,819,192.0 ns | 2,208,277.6 ns | **0.08x** |