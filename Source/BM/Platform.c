#include <GX/Defs.h>

#ifdef CTRGX_BAREMETAL

#include <debug.h> // panic

void ctrgx_platform_break(void) { panic(); }

void ctrgx_platform_log(const char* s, size_t size) {
#ifndef NDEBUG
    // TODO
#endif // !NDEBUG
}

#endif // CTRGX_BAREMETAL