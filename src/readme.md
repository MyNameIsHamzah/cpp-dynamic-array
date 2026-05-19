# Dynamic Array

A custom dynamic array implementation in C++, built from scratch as part of an ongoing C++ systems study.

## Overview

This project reimplements the core behaviour of a heap-allocated, resizable array — similar to `std::vector` — to build understanding of manual memory management, pointer arithmetic, and dynamic allocation from the ground up.

## Structure

```
src/
├── dynamic_array.cpp   # Core implementation
├── CMakeLists.txt      # Build config
├── .clang-format       # Formatting rules
└── .gitignore
```

## Build

```bash
mkdir build && cd build
cmake ..
make
```

## Status

Work in progress — implementation grows alongside C++ study. Expect features and internals to evolve.

## Goals

- Manual memory management (`new` / `delete`)
- Dynamic resizing with amortised O(1) push
- Bounds checking
- Iterator support (planned)