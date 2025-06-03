#include <GX/Defs.h>

#include <stdlib.h> // abort

static size_t findChar(const char* s, char c) {
    size_t index = 0;
    while (s[index] != c) {
        if (!s[index])
            return -1;

        ++index;
    }

    return index;
}

void ctrgx_platform_break(const char* msg, size_t size) {
    size_t pos = findChar(msg, '\n');
    while (pos != -1) {
        svcOutputDebugString(msg, pos);
        msg = msg + pos + 1;
        pos = findChar(msg, '\n');
    }
    
    svcOutputDebugString(msg, findChar(msg, '\0'));
    svcBreak(USERBREAK_PANIC);
    while (true) {}
}