cmake_minimum_required(VERSION 3.14)
project(cpp-dynamic-array VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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