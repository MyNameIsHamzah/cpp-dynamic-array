# dynamicArray

A low-latency, allocator-aware C++20 dynamic array container built with mechanical sympathy, custom monotonic arena memory management, and a bit-packed boolean specialization.

---

## Quick API Overview

```cpp
#include "dynamic_array.hpp"
#include "arena_allocator.hpp"
#include <algorithm>
#include <iostream>

int main() {
    // 1. Standard Contiguous Container (3-pointer layout)
    dynamicArray<int> arr;
    arr.reserve(1024);
    arr.push_back(42);
    arr.emplace_back(17);
    std::ranges::sort(arr); // Satisfies std::contiguous_iterator

    // 2. Zero-Syscall Bump Allocator (Low-latency order lifecycles)
    Arena arena(64 * 1024); // 64 KB pre-allocated arena buffer
    dynamicArray<int, ArenaAllocator<int>> fast_arr{ArenaAllocator<int>(&arena)};
    for (int i = 0; i < 100; ++i) fast_arr.push_back(i);
    arena.reset(); // O(1) instant reset, zero kernel free() syscalls

    // 3. Bit-Packed Boolean Specialization (8x memory compression)
    dynamicArray<bool> flags;
    flags.push_back(true);
    flags.push_back(false);
    flags[1].flip(); // Proxy reference bit manipulation
}
```

---

## Architectural Highlights

* **3-Pointer Layout (`m_begin`, `m_end`, `m_cap`)**: Replaces the classic `pointer + size + capacity` struct. `end()` is a single pointer load, and the capacity check on `push_back` is a single pointer comparison with zero arithmetic.
* **Allocator-Aware RAII (`std::allocator_traits`)**: Completely decouples raw memory allocation from object construction (placement `new`) and destruction. Uses `[[no_unique_address]]` for zero-overhead with stateless allocators.
* **Strong Exception Safety**: During geometric reallocation, new elements are constructed in the new buffer *before* relocating existing elements. This eliminates corruption if constructors throw and safely handles self-referential appends (`arr.push_back(arr[0])`).
* **Hot/Cold Growth Path Splitting**: Inlines the common `push_back` / `emplace_back` fast path while marking the slow reallocation path `[[gnu::noinline]]` to keep CPU instruction cache footprint minimal.
* **C++20 Contiguous & Random-Access Iterators**: Modeled as raw pointer wrappers satisfying `std::contiguous_iterator` and `std::random_access_iterator`, unlocking full compiler auto-vectorization and SIMD loop unrolling.
* **Bit-Packed `dynamicArray<bool>`**: Packs 8 booleans per byte using proxy references with a const/non-const taxonomy split, retaining STL random-access iterator compatibility.
* **Monotonic `ArenaAllocator`**: A 64-byte aligned, deterministic bump allocator with $O(1)$ block resets, designed for bursty lifecycles without OS kernel memory locks.

---

## Performance & Benchmark Findings (EPCC Cirrus Supercomputer)

All benchmarks were compiled with `GCC 12.2 -O3 -march=native` and executed on a dedicated compute node (`cs-n0004`, 3.75 GHz, 32 MB L3 cache) on the **Cirrus HPC Cluster**, pinned to an isolated physical core (`taskset -c 0`) to eliminate OS scheduling jitter and frequency scaling noise.

### Benchmark Charts (PDF)

* **[PushBack Throughput Chart (PDF)](benchmarks/charts/throughput_comparison.pdf)** — Geometric growth vs reserved pre-allocation across $N = 64$ to $10^6$ elements.
* **[Allocator Latency Chart (PDF)](benchmarks/charts/allocator_latency.pdf)** — Cycle latency comparison: `std::allocator` (`malloc`/`free`) vs `ArenaAllocator`.
* **[Boolean Traversal Scan Chart (PDF)](benchmarks/charts/bool_crossover.pdf)** — Memory bandwidth vs bit-masking ALU trade-offs across byte and bit-packed layouts.
* **[Algorithms & Iteration Chart (PDF)](benchmarks/charts/algorithms_comparison.pdf)** — Sequential iteration and `std::sort` validation against `libstdc++`.

### Key Trade-Off Takeaways

* **Zero-Cost Abstraction Parity**: Sequential scans and `std::sort` match `std::vector` down to **<0.2% variance** across $10^6$ elements ($280.3\ \mu\text{s}$ vs $279.9\ \mu\text{s}$), validating that custom contiguous iterators lower directly to raw pointer arithmetic.
* **Burst Allocator Speedup**: Backing `dynamicArray` with `ArenaAllocator` cuts allocation latency by **41% (1.7x speedup)** over `malloc`/`free` at 64 pushes ($61.4\text{ ns}$ vs $105.4\text{ ns}$) by eliminating kernel heap locks and metadata tracking.
* **Boolean Memory vs ALU Trade-off**: `dynamicArray<bool>` compresses memory footprint by **8x**, but bit-shift/mask operations (`(word >> bit) & 1`) make byte-aligned `uint8_t` arrays **1.6x–2.3x faster** for in-cache sequential scans ($1.12\text{ ms}$ vs $1.74\text{ ms}$ at $4\text{M}$ elements).
* **Page Fault Boundary**: Under geometric growth, `std::vector` exhibits a steep latency jump approaching $N = 10^6$ as allocation exceeds `glibc`'s `MMAP_THRESHOLD`, triggering kernel `mmap` page allocations and minor page faults.

> Full timing metrics and speedup multipliers are documented in [benchmarks/BENCHMARK_RESULTS.md](benchmarks/BENCHMARK_RESULTS.md).

---

## Build, Test & Profile

Requires C++20 (`clang++` or `g++`) and CMake 3.14+:

```bash
mkdir -p build && cd build

# 1. Debug build with AddressSanitizer & UB Sanitizer (32 Google Tests)
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON
cmake --build .
ctest --output-on-failure

# 2. Release build with Google Benchmark
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_ASAN=OFF -DBUILD_BENCHMARKS=ON
cmake --build . --target benchmark_suite

# Run benchmarks and generate PDF plots
./benchmark_suite --benchmark_out=../benchmark_results.json --benchmark_out_format=json
python3 ../scripts/plot_benchmarks.py ../benchmark_results.json
```

To submit a batch run on Cirrus Slurm:
```bash
sbatch scripts/cirrus_bench.slurm
```