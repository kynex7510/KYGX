#include <GX/Allocator.h>
#include "../QTMRAM.h"

#include <stdlib.h> // malloc, free
#include <malloc.h> // malloc_usable_size

void* kygxAllocAligned(GXMemType memType, size_t size, size_t alignment) {
    if (!alignment) {
        switch (memType) {
            case GX_MEM_HEAP:
                return malloc(size);
            case GX_MEM_LINEAR:
                return linearAlloc(size);
            case GX_MEM_VRAM:
                return vramAlloc(size);
            case GX_MEM_QTMRAM:
                return qtmramAlloc(size);
            default:
                return NULL;
        }
    }

    switch (memType) {
        case GX_MEM_HEAP:
            return memalign(alignment, size);
        case GX_MEM_LINEAR:
            return linearMemAlign(size, alignment);
        case GX_MEM_VRAM:
            return vramMemAlign(size, alignment);
        case GX_MEM_QTMRAM:
            return qtmramMemAlign(size, alignment);
        default:
            return NULL;
    }
}

KYGX_INLINE vramAllocPos getVRAMPos(GXVRAMBank bank) {
    if (bank == GX_ALLOC_VRAM_BANK_A)
        return VRAM_ALLOC_A;

    if (bank == GX_ALLOC_VRAM_BANK_B)
        return VRAM_ALLOC_B;

    return VRAM_ALLOC_ANY;
}

void* kygxAllocAlignedVRAM(GXVRAMBank bank, size_t size, size_t aligment) {
    if (!aligment)
        return vramAllocAt(size, getVRAMPos(bank));
    
    return vramMemAlignAt(size, aligment, getVRAMPos(bank));
}

void kygxFree(void* p) {
    switch (kygxGetMemType(p)) {
        case GX_MEM_HEAP:
            free(p);
            break;
        case GX_MEM_LINEAR:
            linearFree(p);
            break;
        case GX_MEM_VRAM:
            vramFree(p);
            break;
        case GX_MEM_QTMRAM:
            qtmramFree(p);
            break;
        default:;
    }
}

GXMemType kygxGetMemType(const void* p) {
    const u32 addr = (u32)p;

    if (addr >= OS_HEAP_AREA_BEGIN && addr <= OS_HEAP_AREA_END)
        return GX_MEM_HEAP;

    if (addr >= OS_FCRAM_VADDR && addr <= (OS_FCRAM_VADDR + OS_FCRAM_SIZE))
        return GX_MEM_LINEAR;

    if (addr >= OS_VRAM_VADDR && addr <= (OS_VRAM_VADDR + OS_VRAM_SIZE))
        return GX_MEM_VRAM;

    if (addr >= OS_QTMRAM_VADDR && addr <= (OS_QTMRAM_VADDR + OS_QTMRAM_SIZE))
        return GX_MEM_QTMRAM;

    return GX_MEM_UNKNOWN;
}

size_t kygxGetAllocSize(const void* p) {
    switch (kygxGetMemType(p)) {
        case GX_MEM_HEAP:
            return malloc_usable_size((void*)p);
        case GX_MEM_LINEAR:
            return linearGetSize((void*)p);
        case GX_MEM_VRAM:
            return vramGetSize((void*)p);
        case GX_MEM_QTMRAM:
            return qtmramGetSize(p);
        default:
            return 0;
    }
}

u32 kygxGetPhysicalAddress(const void* addr) { osConvertVirtToPhys(addr); }

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