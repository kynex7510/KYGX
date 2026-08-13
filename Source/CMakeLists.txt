# Enable ARM11 compilation for baremetal.
if (CTR_BM)
    ctr_bm_enable_arm11()
endif()

set(GX_SOURCES
    BatchQueue.c
    BatchRunner.c
    StateMachine.c
)

if (CTR_BM)
    set(GX_SOURCES ${GX_SOURCES}
        libn3ds/CmdQueue.c
        libn3ds/Interrupt.c
    )
else()
    set(GX_SOURCES ${GX_SOURCES}
        libctru/CmdQueue.c
        libctru/Interrupt.c
    )
endif()

add_library(kygx STATIC ${GX_SOURCES})
target_link_libraries(kygx PUBLIC ctr11)
target_include_directories(kygx PUBLIC ../Include PRIVATE .)
target_compile_options(kygx PRIVATE -Wall -Werror)

install(TARGETS kygx)
install(DIRECTORY ../Include/ DESTINATION include)