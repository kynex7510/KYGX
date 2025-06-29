# Check for libn3ds.
if (NOT LIBN3DS_ROOT)
    if (NOT DEFINED ENV{LIBN3DS_ROOT})
        message(FATAL_ERROR "LIBN3DS_ROOT variable not set!")
    else()
        set(LIBN3DS_ROOT $ENV{LIBN3DS_ROOT})
    endif()
endif()

# Check enabled languages.
get_property(ENABLED_LANGS GLOBAL PROPERTY ENABLED_LANGUAGES)

foreach(LANG C CXX ASM)
    if(NOT ${LANG} IN_LIST ENABLED_LANGS)
        message(FATAL_ERROR "Language ${LANG} is not enabled!")
    endif()
endforeach()