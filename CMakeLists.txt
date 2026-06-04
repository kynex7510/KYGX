cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
include(CMake/CPM.cmake)

# C23, CXX, ASM required for libn3ds.
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(kygx C CXX ASM)

option(KYGX_ENABLE_TESTS "Enable tests compilation" OFF)

# ctrcompat.
CPMAddPackage("gh:kynex7510/ctrcompat#5b5c68c")

# Setup library.
add_subdirectory(Source)

# Setup tests.
if (KYGX_ENABLE_TESTS)
    add_subdirectory(Tests)
endif()