cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
include(CMake/Overrides.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(gx)

# Library

file(GLOB GX_SOURCES Source/*.c Source/HOS/*.c)
add_library(gx STATIC ${GX_SOURCES})
target_include_directories(gx PUBLIC Include)
target_compile_options(gx PRIVATE -Wall -Werror)

install(TARGETS gx)
install(DIRECTORY Include/ DESTINATION include)

# Tests

add_subdirectory(Tests)