include(Common)

# Configure variables.

set(CMAKE_C_FLAGS "${ARM9_FLAGS} -mword-relocations -ffunction-sections")
set(CMAKE_CXX_FLAGS "${ARM9_FLAGS} -fno-rtti -fno-exceptions -mword-relocations -ffunction-sections")
set(CMAKE_ASM_FLAGS "${ARM9_FLAGS} -x assembler-with-cpp")
set(CMAKE_EXE_LINKER_FLAGS ${ARM9_LINKER_FLAGS})

# Set libn3ds files.

set(LIBN3DS_SOURCE_PATHS
    ${LIBN3DS_ROOT}/source
    ${LIBN3DS_ROOT}/source/drivers/mmc
    ${LIBN3DS_ROOT}/source/drivers
    ${LIBN3DS_ROOT}/source/arm9
    ${LIBN3DS_ROOT}/source/arm9/drivers
    ${LIBN3DS_ROOT}/libraries/fatfs/source
    ${LIBN3DS_ROOT}/source/arm9/fatfs
)

set(LIBN3DS_SOURCES)

foreach(DIR IN LISTS LIBN3DS_SOURCE_PATHS)
    foreach(EXT c cpp s)
        file(GLOB TMP_FILES CONFIGURE_DEPENDS "${DIR}/*.${EXT}")
        list(APPEND LIBN3DS_SOURCES ${TMP_FILES})
    endforeach()
endforeach()

set(LIBN3DS_INCLUDE_PATHS
    ${LIBN3DS_ROOT}/include
    ${LIBN3DS_ROOT}/libraries
    ${LIBN3DS_ROOT}/source/arm9/fatfs
)

add_library(n3ds9 OBJECT ${LIBN3DS_SOURCES})
target_include_directories(n3ds9 PUBLIC ${LIBN3DS_INCLUDE_PATHS})