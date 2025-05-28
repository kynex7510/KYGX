cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

# Check for devkitPro.
if (NOT DEVKITPRO)
    if (NOT DEFINED ENV{DEVKITPRO})
        message(FATAL_ERROR "DEVKITPRO variable not set!")
    else()
        set(DEVKITPRO $ENV{DEVKITPRO})
    endif()
endif()

# Set module path.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")

# Set system variables.
set(CMAKE_SYSTEM_NAME Generic-ELF)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR armv6k)

# Setup devkitARM.
include(${DEVKITPRO}/cmake/devkitARM.cmake)

# Set default arch flags.
set(ARM9_FLAGS "-march=armv5te -mtune=arm946e-s -mfloat-abi=soft -mtp=soft -marm -mthumb-interwork -masm-syntax-unified -D__ARM9__ -D__3DS__")
set(ARM11_FLAGS "-march=armv6k+vfpv2 -mtune=mpcore -mfloat-abi=hard -mtp=soft -marm -mthumb-interwork -masm-syntax-unified -D__ARM11__ -D__3DS__")

# Set default linker flags.
# TODO: map
set(ARM9_LINKER_FLAGS "${ARM9_FLAGS} -Wl,-T,${CMAKE_SOURCE_DIR}/CMake/ARM9.ld -Wl,-d -Wl,--use-blx -Wl,--gc-sections -nostartfiles")
set(ARM11_LINKER_FLAGS "${ARM11_FLAGS} -Wl,-T,${CMAKE_SOURCE_DIR}/CMake/ARM11.ld -Wl,-d -Wl,--use-blx -Wl,--gc-sections -nostartfiles")