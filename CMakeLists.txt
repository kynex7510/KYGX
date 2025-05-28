cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

# C23, CXX, ASM required for libn3ds.
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(gx C CXX ASM)

option(CTRGX_BAREMETAL "Enable baremetal compilation" OFF)
option(CTRGX_ENABLE_TESTS "Enable tests compilation" OFF)

# Include libn3ds for baremetal compilation.
if (CTRGX_BAREMETAL)
    include(ARM11)
endif()

# Setup library.
set(GX_SOURCES
    Source/Command.c
    Source/CommandBuffer.c
    Source/GX.c
    Source/Interrupt.c
)

if (CTRGX_BAREMETAL)
    set(GX_SOURCES ${GX_SOURCES} Source/BM/Platform.c)
else()
    set(GX_SOURCES ${GX_SOURCES} Source/HOS/Platform.c)
endif()

add_library(gx STATIC ${GX_SOURCES})

if (CTRGX_BAREMETAL)
    target_include_directories(gx PUBLIC ${LIBN3DS_INCLUDE_PATHS})
    add_compile_definitions(CTRGX_BAREMETAL)
endif()

target_include_directories(gx PUBLIC Include)
target_compile_options(gx PRIVATE -Wall -Werror)

install(TARGETS gx)
install(DIRECTORY Include/ DESTINATION include)

# Setup tests.
if (CTRGX_ENABLE_TESTS)
    add_subdirectory(Tests)
endif()