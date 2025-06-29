function(make_firm OUTPUT_NAME)
    set(OPTS)
    set(SINGLE)
    set(MULTI ARM9 ARM11)
    cmake_parse_arguments(MAKE_FIRM "${OPTS}" "${SINGLE}" "${MULTI}" ${ARGN})

    if(NOT MAKE_FIRM_ARM9 OR NOT MAKE_FIRM_ARM11)
        message(FATAL_ERROR "make_firm: Both ARM9 and ARM11 inputs are required")
    endif()

    # Prepare ARM9 variables.
    list(GET MAKE_FIRM_ARM9 0 ARM9_TARGET)
    list(GET MAKE_FIRM_ARM9 1 ARM9_LOAD_ADDRESS)
    list(GET MAKE_FIRM_ARM9 2 ARM9_ENTRYPOINT)
    list(GET MAKE_FIRM_ARM9 3 ARM9_COPY_METHOD)
    get_target_property(ARM9_FULL_PATH ${ARM9_TARGET} FULL_PATH)

    # Prepare ARM11 variables.
    list(GET MAKE_FIRM_ARM11 0 ARM11_TARGET)
    list(GET MAKE_FIRM_ARM11 1 ARM11_LOAD_ADDRESS)
    list(GET MAKE_FIRM_ARM11 2 ARM11_ENTRYPOINT)
    list(GET MAKE_FIRM_ARM11 3 ARM11_COPY_METHOD)
    get_target_property(ARM11_FULL_PATH ${ARM11_TARGET} FULL_PATH)

    # Add FIRM target.
    add_custom_target(${OUTPUT_NAME} ALL
        COMMAND firmtool build ${OUTPUT_NAME}.firm
            -D ${ARM9_FULL_PATH} ${ARM11_FULL_PATH}
            -A ${ARM9_LOAD_ADDRESS} ${ARM11_LOAD_ADDRESS}
            -n ${ARM9_ENTRYPOINT} -e ${ARM11_ENTRYPOINT}
            -C ${ARM9_COPY_METHOD} ${ARM11_COPY_METHOD}
        DEPENDS ${ARM9_TARGET} ${ARM11_TARGET}
    )

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_NAME}.firm TYPE BIN)
endfunction()

function(make_firm_binary TARGET_NAME OUTPUT_NAME)
    # Add binary target.
    add_custom_target(${OUTPUT_NAME} ALL
        COMMAND ${DKP_OBJCOPY} -O binary $<TARGET_FILE:${TARGET_NAME}> ${OUTPUT_NAME}.bin
        DEPENDS ${TARGET_NAME}
    )

    set_target_properties(${OUTPUT_NAME} PROPERTIES
        FULL_PATH ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_NAME}.bin
    )
endfunction()