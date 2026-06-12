cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
include(CMake/CPM.cmake)

# C23, CXX, ASM required for libn3ds.
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(kygx C CXX ASM)

option(KYGX_ENABLE_TESTS "Enable tests compilation" OFF)
option(KYGX_ENABLE_DOCS "Enable docs generation" OFF)

# Setup dependencies.
CPMAddPackage("gh:kynex7510/ctrcompat#5b5c68c")
find_package(Doxygen)

# Setup docs.
if (KYGX_ENABLE_DOCS)
    if (DOXYGEN_FOUND)
        set(DOXYGEN_GENERATE_HTML YES)
        set(DOXYGEN_GENERATE_LATEX NO)
        set(DOXYGEN_EXTRACT_ALL YES)
        set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "./README.md")
        doxygen_add_docs(
            docs
            ./Include
            ./README.md
            COMMENT "Generating API documentation with Doxygen"
        )
    else()
        message(FATAL_ERROR "Doxygen is required for generating docs, but couldn't be found")
    endif()
endif()

# Setup library.
add_subdirectory(Source)

# Setup tests.
if (KYGX_ENABLE_TESTS)
    add_subdirectory(Tests)
endif()