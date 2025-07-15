cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

# C23, CXX, ASM required for libn3ds.
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(kygx C CXX ASM)

option(KYGX_ENABLE_TESTS "Enable tests compilation" OFF)

# Include libn3ds for baremetal compilation.
if (CTR_BAREMETAL)
    include(BM/N3DS11)
    message(STATUS "KYGX: Compiling for baremetal")
else()
    # Hack that allows us to use a fork of libctru.
    include(CMake/CPM.cmake)
    set(CMAKE_C_STANDARD_LIBRARIES "-lm" CACHE STRING "" FORCE)
    set(CMAKE_CXX_STANDARD_LIBRARIES "-lm" CACHE STRING "" FORCE)
    set(CMAKE_ASM_STANDARD_LIBRARIES "-lm" CACHE STRING "" FORCE)
    CPMAddPackage("gh:kynex7510/libctru#c336131")
    link_libraries(ctru)
    message(STATUS "KYGX: Compiling for userland (with libctru fork)")
endif()

# Setup library.
set(GX_SOURCES Source/GX.c)

if (CTR_BAREMETAL)
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

if (CTR_BAREMETAL)
    target_include_directories(kygx PUBLIC ${LIBN3DS_INCLUDE_PATHS})
    target_compile_definitions(kygx PUBLIC KYGX_BAREMETAL)
    target_link_libraries(kygx PUBLIC n3ds11)
endif()

target_include_directories(kygx PUBLIC Include)
target_compile_options(kygx PRIVATE -Wall -Werror)

install(TARGETS kygx)
install(DIRECTORY Include/ DESTINATION include)

# Setup tests.
if (KYGX_ENABLE_TESTS)
    add_subdirectory(Tests)
endif()