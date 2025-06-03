#include <GX/Defs.h>

#ifndef CTRGX_BAREMETAL

#include <stdlib.h> // abort

void ctrgx_platform_break(void) {
    svcBreak(USERBREAK_PANIC);
    while (true) {}
}

void ctrgx_platform_log(const char* s, size_t size) {
#ifndef NDEBUG
    svcOutputDebugString(s, size);
#endif // !NDEBUG
}

#endif // !CTRGX_BAREMETAL