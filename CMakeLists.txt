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
set(GX_SOURCES
    Source/BatchQueue.c
    Source/StateMachine.c
    Source/String.c
)

if (CTR_BAREMETAL)
    set(GX_SOURCES ${GX_SOURCES} Source/libn3ds/GXServer.c)
else()
    set(GX_SOURCES ${GX_SOURCES} Source/libctru/GXServer.c)
endif()

add_library(kygx STATIC ${GX_SOURCES})
target_link_libraries(kygx PUBLIC ctrcompat)
target_include_directories(kygx PUBLIC Include PRIVATE Source)
target_compile_options(kygx PRIVATE -Wall -Werror)

install(TARGETS kygx)
install(DIRECTORY Include/ DESTINATION include)

# Setup tests.
if (KYGX_ENABLE_TESTS)
    add_subdirectory(Tests)
endif()