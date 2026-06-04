# Enable ARM11 compilation for baremetal.
if (CTR_BM)
    ctr_bm_enable_arm11()
endif()

set(GX_SOURCES
    BatchQueue.c
    StateMachine.c
)

if (CTR_BM)
    set(GX_SOURCES ${GX_SOURCES} libn3ds/GXServer.c)
else()
    set(GX_SOURCES ${GX_SOURCES} libctru/GXServer.c)
endif()

add_library(kygx STATIC ${GX_SOURCES})
target_link_libraries(kygx PUBLIC ctrcompat)
target_include_directories(kygx PUBLIC ../Include PRIVATE .)
target_compile_options(kygx PRIVATE -Wall -Werror)

install(TARGETS kygx)
install(DIRECTORY Include/ DESTINATION include)