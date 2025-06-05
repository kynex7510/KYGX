cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

# C23, CXX, ASM required for libn3ds.
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(kygx C CXX ASM)

option(KYGX_BAREMETAL "Enable baremetal compilation" OFF)
option(KYGX_ENABLE_TESTS "Enable tests compilation" OFF)

# Include libn3ds for baremetal compilation.
if (KYGX_BAREMETAL)
    include(ARM11)
endif()

# Setup library.
set(GX_SOURCES Source/GX.c)

if (KYGX_BAREMETAL)
    set(GX_SOURCES ${GX_SOURCES}
        Source/BM/Platform.c
        Source/BM/Allocator.c
    )
else()
    set(GX_SOURCES ${GX_SOURCES}
        Source/HOS/Platform.c
        Source/HOS/Allocator.c
    )
endif()

add_library(kygx STATIC ${GX_SOURCES})

if (KYGX_BAREMETAL)
    target_include_directories(kygx PUBLIC ${LIBN3DS_INCLUDE_PATHS})
    add_compile_definitions(KYGX_BAREMETAL)
endif()

target_include_directories(kygx PUBLIC Include)
target_compile_options(kygx PRIVATE -Wall -Werror)

install(TARGETS kygx)
install(DIRECTORY Include/ DESTINATION include)

# Setup tests.
if (KYGX_ENABLE_TESTS)
    add_subdirectory(Tests)
endif()