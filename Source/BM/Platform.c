#include <debug.h> // panicMsg

#include <GX/Defs.h>

void ctrgx_platform_break(const char* msg, size_t size) {
    (void)size;
    panicMsg(msg);
}