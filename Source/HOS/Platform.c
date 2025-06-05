#include <GX/Defs.h>

void ctrgx_platform_yield(void) { svcSleepThread(0); }

#ifndef NDEBUG
static size_t findChar(const char* s, char c) {
    size_t index = 0;
    while (s[index] != c) {
        if (!s[index])
            return -1;

        ++index;
    }

    return index;
}

void ctrgx_platform_break(const char* msg) {
    if (msg) {
        size_t pos = findChar(msg, '\n');
        while (pos != -1) {
            svcOutputDebugString(msg, pos);
            msg = msg + pos + 1;
            pos = findChar(msg, '\n');
        }
        
        svcOutputDebugString(msg, findChar(msg, '\0'));
    }

    svcBreak(USERBREAK_PANIC);
    while (true) {}
}

#else

void ctrgx_platform_break(const char* msg) {
    (void)msg;
    svcBreak(USERBREAK_PANIC);
    while (true) {}
}

#endif // !NDEBUG