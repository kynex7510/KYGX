#ifdef KYGX_BAREMETAL
#include <mem_map.h>
#endif // KYGX_BAREMETAL

#include <KYGX/Allocator.h>

#include <stdlib.h> // realloc
#include <string.h> // memcpy

static void* genericRealloc(KYGXMemType type, void* p, size_t size) {
    KYGX_ASSERT(p);

    void* q = kygxAlloc(type, size);
    if (q) {
        const size_t oldSize = kygxGetAllocSize(p);
        memcpy(q, p, size < oldSize ? size : oldSize);
        kygxFree(p);
    }

    return q;
}

static inline void* vramReallocCustom(void* p, size_t newSize) {
    // Detect bank.
#ifdef KYGX_BAREMETAL
    const size_t baseB = VRAM_BANK1;
#else
    const size_t baseB = OS_VRAM_VADDR + OS_VRAM_SIZE / 2;
#endif

    const KYGXVRAMBank bank = (u32)p >= baseB ? KYGX_ALLOC_VRAM_BANK_B : KYGX_ALLOC_VRAM_BANK_A;

    // If the new size is less than the old size, reallocation must succeed.
    const size_t oldSize = kygxGetAllocSize(p);
    if (newSize < oldSize) {
        kygxFree(p);
        p = kygxAllocVRAM(bank, newSize);
        KYGX_ASSERT(p);
        return p;
    }

    // Try to realloc memory in the same bank first.
    void* q = kygxAllocVRAM(bank, newSize);
    if (!q)
        q = kygxAllocVRAM(bank == KYGX_ALLOC_VRAM_BANK_A ? KYGX_ALLOC_VRAM_BANK_B : KYGX_ALLOC_VRAM_BANK_A, newSize);

    if (q)
        kygxFree(p);

    return q;
}

void* kygxRealloc(void* p, size_t newSize) {
    if (newSize == 0) {
        kygxFree(p);
        return NULL;
    }

    switch (kygxGetMemType(p)) {
        case KYGX_MEM_HEAP:
            // We assume both HOS and BM to use malloc/free for heap (which is true).
            return realloc(p, newSize);
        case KYGX_MEM_LINEAR:
            return genericRealloc(KYGX_MEM_LINEAR, p, newSize);
        case KYGX_MEM_VRAM:
            return vramReallocCustom(p, newSize);
        case KYGX_MEM_QTMRAM:
            return genericRealloc(KYGX_MEM_QTMRAM, p, newSize);
        default:
            return NULL;
    }
}