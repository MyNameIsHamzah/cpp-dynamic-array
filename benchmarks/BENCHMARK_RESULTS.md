# Cirrus HPC Benchmark Results & Performance Analysis

Benchmarked on a dedicated compute node (`cs-n0004`, 576 CPUs, 3.75 GHz, 32 MB L3 Cache) on the **Cirrus Supercomputer (EPCC)**. Pinned to core 0 via `taskset` to eliminate thread scheduling jitter and frequency scaling noise.

## 1. Container PushBack Throughput (Time per full sequence)

| Elements (N) | dynamicArray (Geometric) | std::vector (Geometric) | Speedup vs STL | dynamicArray (Reserved) | std::vector (Reserved) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| 64 | 103.6 ns | 110.2 ns | **1.06x (FASTER)** | 37.7 ns | 34.4 ns |
| 256 | 239.3 ns | 253.5 ns | **1.06x (FASTER)** | 180.1 ns | 132.5 ns |
| 1,024 | 807.9 ns | 822.1 ns | **1.02x (FASTER)** | 649.0 ns | 431.4 ns |
| 4,096 | 2,778.5 ns | 2,655.7 ns | 0.96x | 2,422.4 ns | 1,566.4 ns |
| 16,384 | 10,305.5 ns | 9,861.6 ns | 0.96x | 9,616.0 ns | 6,063.5 ns |
| 65,536 | 88,365.4 ns | 38,351.2 ns | 0.43x | 38,343.3 ns | 24,226.1 ns |
| 262,144 | 410,035.6 ns | 156,883.9 ns | 0.38x | 153,110.5 ns | 97,785.1 ns |
| 1,048,576 | 1,444,564.6 ns | 1,453,382.7 ns | **1.01x (FASTER)** | 612,650.2 ns | 386,751.6 ns |

## 2. Iteration & std::sort Performance (Validating 3-Pointer & Contiguous Iterators)

| Elements (N) | dynamicArray Iteration | std::vector Iteration | dynamicArray std::sort | std::vector std::sort |
| :--- | :--- | :--- | :--- | :--- |
| 1,024 | 245.1 ns | 229.9 ns | 4,192.5 ns | 4,089.6 ns |
| 4,096 | 1,063.8 ns | 1,049.0 ns | 20,414.6 ns | 19,959.7 ns |
| 16,384 | 4,340.7 ns | 4,324.7 ns | 745,126.9 ns | 735,776.5 ns |
| 65,536 | 17,442.8 ns | 17,428.1 ns | 3,626,978.4 ns | 3,579,394.6 ns |
| 262,144 | 69,904.2 ns | 69,896.6 ns | N/A | N/A |
| 1,048,576 | 280,306.8 ns | 279,955.6 ns | N/A | N/A |

## 3. Allocator Impact: std::allocator (Heap malloc) vs ArenaAllocator (0-Syscall Bump)

| Cycle Elements | std::allocator (malloc/free) | ArenaAllocator (Bump + Reset) | Speedup (Latency Win) |
| :--- | :--- | :--- | :--- |
| 64 pushes | 105.4 ns | 61.4 ns | **1.72x faster** |
| 256 pushes | 241.4 ns | 175.7 ns | **1.37x faster** |
| 1,024 pushes | 809.9 ns | 676.7 ns | **1.20x faster** |
| 4,096 pushes | 2,778.8 ns | 2,557.4 ns | **1.09x faster** |

## 4. The Boolean Specialization (Full Traversal Scan)

| Total Booleans | dynamicArray<uint8_t> (Byte) | dynamicArray<bool> (1-bit Packed) | std::vector<bool> (STL) |
| :--- | :--- | :--- | :--- |
| 64 | 11.9 ns | 27.0 ns | 26.7 ns |
| 256 | 49.4 ns | 115.2 ns | 111.9 ns |
| 1,024 | 245.2 ns | 436.9 ns | 424.0 ns |
| 4,096 | 1,064.0 ns | 1,721.0 ns | 1,670.7 ns |
| 16,384 | 4,339.7 ns | 7,016.4 ns | 7,005.2 ns |
| 65,536 | 17,443.4 ns | 27,782.0 ns | 28,023.9 ns |
| 262,144 | 69,852.5 ns | 111,121.5 ns | 106,667.6 ns |
| 1,048,576 | 279,508.1 ns | 444,877.9 ns | 452,810.4 ns |
| 4,194,304 | 1,120,106.6 ns | 1,737,736.9 ns | 1,740,212.2 ns |