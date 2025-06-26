#include <GX/Defs.h>

void kygx_platform_yield(void) { svcSleepThread(0); }

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

void kygx_platform_break(const char* msg) {
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

void kygx_platform_break(const char* msg) {
    (void)msg;
    svcBreak(USERBREAK_PANIC);
    while (true) {}
}

#endif // !NDEBUG

void kygxInvalidateDataCache(void* addr, size_t size) {
    // GSP will return an error if the address is not in FCRAM/VRAM.
    const bool fcram = (u32)addr >= OS_FCRAM_VADDR && (u32)addr <= (OS_FCRAM_VADDR + OS_FCRAM_SIZE);
    const bool vram = (u32)addr >= OS_VRAM_VADDR && (u32)addr <= (OS_VRAM_VADDR + OS_VRAM_SIZE);

    if (fcram || vram) {
        KYGX_BREAK_UNLESS(R_SUCCEEDED(GSPGPU_InvalidateDataCache(addr, size)));
    }
}