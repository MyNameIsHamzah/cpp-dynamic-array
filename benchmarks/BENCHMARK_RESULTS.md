# Cirrus HPC Benchmark Results & Performance Analysis

Benchmarked on an exclusive **Cirrus HPC** dual-socket **AMD EPYC 9825** compute node (`cs-n0004`; 288 physical cores / 576 SMT threads @ 3.71 GHz max boost, NPS4 NUMA topology). Pinned to a single physical core (`core 0`) via `taskset -c 0` within an isolated 12-core CCD (48 KiB L1d, 1 MiB L2, 32 MiB local L3 slice) to eliminate cross-CCD infinity fabric latency, SMT contention, and OS scheduling jitter.

## 1. Container PushBack Throughput (Time per full sequence)

| Elements (N) | dynamicArray (Geometric) | std::vector (Geometric) | Speedup vs STL | dynamicArray (Reserved) | std::vector (Reserved) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| 64 | 106.9 ns | 108.8 ns | **1.02x (FASTER)** | 37.8 ns | 34.5 ns |
| 256 | 278.6 ns | 258.4 ns | 0.93x | 180.1 ns | 142.9 ns |
| 1,024 | 910.7 ns | 819.8 ns | 0.90x | 649.7 ns | 428.0 ns |
| 4,096 | 3,141.4 ns | 2,659.5 ns | 0.85x | 2,438.8 ns | 1,547.7 ns |
| 16,384 | 11,605.4 ns | 9,900.2 ns | 0.85x | 9,637.9 ns | 6,051.2 ns |
| 65,536 | 96,148.8 ns | 38,408.2 ns | 0.40x | 38,441.3 ns | 24,411.3 ns |
| 262,144 | 445,544.9 ns | 157,486.4 ns | 0.35x | 153,702.3 ns | 96,025.8 ns |
| 1,048,576 | 1,572,400.9 ns | 1,483,696.8 ns | 0.94x | 614,960.3 ns | 383,360.7 ns |

## 2. Iteration & std::sort Performance (Validating 3-Pointer & Contiguous Iterators)

| Elements (N) | dynamicArray Iteration | std::vector Iteration | dynamicArray std::sort | std::vector std::sort |
| :--- | :--- | :--- | :--- | :--- |
| 1,024 | 245.3 ns | 230.4 ns | 4,205.5 ns | 4,089.4 ns |
| 4,096 | 1,066.4 ns | 1,051.7 ns | 20,493.6 ns | 19,537.6 ns |
| 16,384 | 4,351.1 ns | 4,335.8 ns | 745,511.5 ns | 733,518.9 ns |
| 65,536 | 17,485.6 ns | 17,467.9 ns | 3,611,185.1 ns | 3,596,960.5 ns |
| 262,144 | 70,108.5 ns | 70,078.5 ns | N/A | N/A |
| 1,048,576 | 281,057.1 ns | 281,165.6 ns | N/A | N/A |

## 3. Allocator Impact: std::allocator (Heap malloc) vs ArenaAllocator (0-Syscall Bump)

| Cycle Elements | std::allocator (malloc/free) | ArenaAllocator (Bump + Reset) | Speedup (Latency Win) |
| :--- | :--- | :--- | :--- |
| 64 pushes | 104.3 ns | 61.7 ns | **1.69x faster** |
| 256 pushes | 239.1 ns | 176.5 ns | **1.35x faster** |
| 1,024 pushes | 807.5 ns | 678.2 ns | **1.19x faster** |
| 4,096 pushes | 2,783.8 ns | 2,549.0 ns | **1.09x faster** |

## 4. The Boolean Specialization (Full Traversal Scan)

| Total Booleans | dynamicArray<uint8_t> (Byte) | dynamicArray<bool> (1-bit Packed) | std::vector<bool> (STL) |
| :--- | :--- | :--- | :--- |
| 64 | 11.8 ns | 27.1 ns | 27.7 ns |
| 256 | 49.6 ns | 114.9 ns | 115.3 ns |
| 1,024 | 245.9 ns | 441.3 ns | 449.0 ns |
| 4,096 | 1,066.9 ns | 1,753.3 ns | 1,696.7 ns |
| 16,384 | 4,350.9 ns | 7,003.1 ns | 6,674.2 ns |
| 65,536 | 17,488.7 ns | 27,589.9 ns | 27,195.3 ns |
| 262,144 | 70,024.6 ns | 108,501.1 ns | 112,418.4 ns |
| 1,048,576 | 280,188.0 ns | 444,112.0 ns | 421,372.4 ns |
| 4,194,304 | 1,123,631.1 ns | 1,781,649.7 ns | 1,898,170.6 ns |