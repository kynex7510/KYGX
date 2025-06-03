#include <debug.h> // panic

#include <GX/Defs.h>

void ctrgx_platform_break(void) { panic(); }

void ctrgx_platform_log(const char* s, size_t size) {
#ifndef NDEBUG
    // TODO
#endif // !NDEBUG
}