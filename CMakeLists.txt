cmake_minimum_required(VERSION 3.14)
project(cpp-dynamic-array VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default to Release mode if not specified (crucial for microbenchmarks)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

# Compiler warnings
add_compile_options(-Wall -Wextra -Wpedantic)

# Sanitizers option
option(ENABLE_ASAN "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# Header-only library interface
add_library(dynamic_array_lib INTERFACE)
target_include_directories(dynamic_array_lib INTERFACE src)

# Main demo executable
add_executable(dynamic_array src/dynamic_array.cpp)
target_link_libraries(dynamic_array PRIVATE dynamic_array_lib)

# ---------------------------------------------------------
# Testing with GoogleTest
# ---------------------------------------------------------
option(BUILD_TESTS "Build unit test suite" ON)

if(BUILD_TESTS)
    enable_testing()

    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    # Prevent overriding parent compiler/linker settings on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(unit_tests
        tests/test_lifecycle.cpp
        tests/test_iterators.cpp
        tests/test_bool.cpp
        tests/test_exception_safety.cpp
        tests/test_arena_allocator.cpp
    )
    target_link_libraries(unit_tests PRIVATE dynamic_array_lib GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(unit_tests)
endif()

# ---------------------------------------------------------
# Benchmarking with Google Benchmark
# ---------------------------------------------------------
option(BUILD_BENCHMARKS "Build performance benchmark suite" ON)

if(BUILD_BENCHMARKS)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)

    include(FetchContent)
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.8.4
    )
    FetchContent_MakeAvailable(googlebenchmark)

    add_executable(benchmark_suite
        benchmarks/bench_main.cpp
    )
    target_link_libraries(benchmark_suite PRIVATE dynamic_array_lib benchmark::benchmark_main)
endif()