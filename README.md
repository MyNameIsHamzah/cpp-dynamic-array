# dynamicArray

A custom, low-level dynamic array implementation in modern C++20, built from the ground up to explore manual memory management, pointer arithmetic, allocator awareness, and hardware-sympathetic container design.

## Overview

I started this project as a deep dive into how `std::vector` actually operates under the hood, particularly in latency-critical systems (e.g. trading and low-latency infrastructure). 

After stepping away for a bit to focus on university commitments and coursework, I've jumped back in to revamp the memory model and container internals:
- **3-pointer memory layout**: Swapped out the traditional `pointer + size + capacity` layout for three pointers (`m_begin`, `m_end`, `m_cap`). `end()` is a single pointer load, and the `push_back` capacity check is a single pointer compare.
- **Allocator-aware RAII**: All allocation, deallocation, object construction, and destruction are handled via `std::allocator_traits` with zero-overhead stateless allocators using `[[no_unique_address]]`.
- **Strong exception safety & self-reference safety**: Reallocation constructs the incoming element in the new buffer before relocating existing elements. This cleanly avoids use-after-free bugs if an element is pushed into itself (e.g. `vec.push_back(vec[0])`), and utilises `std::is_nothrow_move_constructible_v` so containers are never corrupted if an operation throws.
- **Hot/cold code splitting**: The growth and reallocation slow path is isolated in a `[[gnu::noinline]]` helper to keep the `emplace_back` fast-path tight and cache-friendly.
- **Bit-packed boolean specialisation**: Initial implementation of `dynamicArray<bool>` using bit-masking and a proxy reference class to pack 8 booleans into a single byte.

## Project Structure

```
cpp-dynamic-array/
├── src/
│   ├── dynamic_array.hpp    # Core container & dynamicArray<bool> specialisation
│   └── dynamic_array.cpp    # Test driver & verification functions
├── docs/
│   └── DESIGN.md            # Detailed technical specs and invariants
├── CMakeLists.txt
├── .clang-format
├── .gitignore
└── README.md
```

## Build & Run

Requires a C++20 compliant compiler (`clang++` or `g++`) and CMake 3.10+:

```bash
mkdir -p build && cd build
cmake ..
cmake --build .
./dynamic_array
```

To run with AddressSanitizer and UndefinedBehaviorSanitizer:

```bash
clang++ -std=c++20 -Wall -Wextra -Wpedantic -fsanitize=address,undefined -Isrc src/dynamic_array.cpp -o build/asan_test
./build/asan_test
```

## Status & What's Next

- [x] 3-pointer layout and full allocator-aware RAII lifecycle
- [x] Exception-safe capacity methods (`reserve`, `shrink_to_fit`, `resize`, `clear`, `pop_back`)
- [x] Self-referential push_back safety and hot/cold split
- [x] Random access & contiguous iterators for `dynamicArray<T>` (satisfying C++20 concepts)
- [x] Random access iterators for `dynamicArray<bool>` proxy types
- [ ] Migrate tests from ad-hoc `void` verification functions to Google Test
- [ ] Custom monotonic arena allocator driver
- [ ] Google Benchmark suite comparing against `std::vector` and evaluating cache vs bitmask trade-offs

*Note on current testing:* Tests in `src/dynamic_array.cpp` are currently structured as basic `void` test routines with assertion checks (tracking object lifetimes, simulated throw points, and memory leaks) to verify invariants while building out the core. Proper GTest integration is planned once iterators are in place.