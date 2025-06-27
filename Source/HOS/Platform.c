#include <KYGX/Defs.h>

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

void kygxInvalidateDataCache(const void* addr, size_t size) {
    // GSP will return an error if the address is not in FCRAM/VRAM.
    const bool fcram = (u32)addr >= OS_FCRAM_VADDR && (u32)addr <= (OS_FCRAM_VADDR + OS_FCRAM_SIZE);
    const bool vram = (u32)addr >= OS_VRAM_VADDR && (u32)addr <= (OS_VRAM_VADDR + OS_VRAM_SIZE);

    if (fcram || vram) {
        KYGX_BREAK_UNLESS(R_SUCCEEDED(GSPGPU_InvalidateDataCache(addr, size)));
    }
}

u32 kygxGetPhysicalAddress(const void* addr) { return osConvertVirtToPhys(addr); }

void* kygxGetVirtualAddress(u32 addr) {
    #define CONVERT_REGION(_name)                                         \
    if (addr >= OS_##_name##_PADDR &&                                     \
        addr < (OS_##_name##_PADDR + OS_##_name##_SIZE))                  \
        return (void*)(addr - (OS_##_name##_PADDR + OS_##_name##_VADDR));

    CONVERT_REGION(FCRAM);
    CONVERT_REGION(VRAM);
    CONVERT_REGION(OLD_FCRAM);
    CONVERT_REGION(DSPRAM);
    CONVERT_REGION(QTMRAM);
    CONVERT_REGION(MMIO);

#undef CONVERT_REGION
    return NULL;
}