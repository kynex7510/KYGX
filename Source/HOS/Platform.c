#include "GX/Defs.h"

#include <stdlib.h> // abort

void ctrgx_platform_break(void) {
    svcBreak(USERBREAK_PANIC);
    __builtin_unreachable();
}

void ctrgx_platform_abort(void) { abort(); }

void ctrgx_platform_log(const char* s, size_t size) {
#ifndef NDEBUG
    svcOutputDebugString(s, size);
#endif // !NDEBUG
}