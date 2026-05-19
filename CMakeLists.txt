cmake_minimum_required(VERSION 3.10.0)
project(cpp-mastery VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Werror -Wpedantic)

# Tell CMake to look for headers inside the current folders
include_directories(src)

# ---------------------------------------------------------
# Build the Executables (Target Name -> Files needed)
# ---------------------------------------------------------
add_executable(dynamic_array  src/dynamic_array.cpp)