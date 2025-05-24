# Override dkP defaults. 
if (NOT DKP_NO_BUILTIN_CMAKE_CONFIGS)
    message(FATAL_ERROR "Invalid configuration, invoke CMake with -DDKP_NO_BUILTIN_CMAKE_CONFIGS")
endif()

foreach(lang IN ITEMS C CXX ASM)
    set(CMAKE_${lang}_FLAGS_DEBUG " -g -Og")
	set(CMAKE_${lang}_FLAGS_MINSIZEREL " -Oz -DNDEBUG")
	set(CMAKE_${lang}_FLAGS_RELEASE " -O2 -DNDEBUG")
	set(CMAKE_${lang}_FLAGS_RELWITHDEBINFO " -g -O2 -DNDEBUG")
endforeach()