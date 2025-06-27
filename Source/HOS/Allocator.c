#include <GX/Allocator.h>
#include "../QTMRAM.h"

#include <stdlib.h> // malloc, free
#include <malloc.h> // malloc_usable_size

void* kygxAllocAligned(KYGXMemType memType, size_t size, size_t alignment) {
    if (!alignment) {
        switch (memType) {
            case KYGX_MEM_HEAP:
                return malloc(size);
            case KYGX_MEM_LINEAR:
                return linearAlloc(size);
            case KYGX_MEM_VRAM:
                return vramAlloc(size);
            case KYGX_MEM_QTMRAM:
                return qtmramAlloc(size);
            default:
                return NULL;
        }
    }

    switch (memType) {
        case KYGX_MEM_HEAP:
            return memalign(alignment, size);
        case KYGX_MEM_LINEAR:
            return linearMemAlign(size, alignment);
        case KYGX_MEM_VRAM:
            return vramMemAlign(size, alignment);
        case KYGX_MEM_QTMRAM:
            return qtmramMemAlign(size, alignment);
        default:
            return NULL;
    }
}

KYGX_INLINE vramAllocPos getVRAMPos(KYGXVRAMBank bank) {
    if (bank == KYGX_ALLOC_VRAM_BANK_A)
        return VRAM_ALLOC_A;

    if (bank == KYGX_ALLOC_VRAM_BANK_B)
        return VRAM_ALLOC_B;

    return VRAM_ALLOC_ANY;
}

void* kygxAllocAlignedVRAM(KYGXVRAMBank bank, size_t size, size_t aligment) {
    if (!aligment)
        return vramAllocAt(size, getVRAMPos(bank));
    
    return vramMemAlignAt(size, aligment, getVRAMPos(bank));
}

void kygxFree(void* p) {
    switch (kygxGetMemType(p)) {
        case KYGX_MEM_HEAP:
            free(p);
            break;
        case KYGX_MEM_LINEAR:
            linearFree(p);
            break;
        case KYGX_MEM_VRAM:
            vramFree(p);
            break;
        case KYGX_MEM_QTMRAM:
            qtmramFree(p);
            break;
        default:;
    }
}

KYGXMemType kygxGetMemType(const void* p) {
    const u32 addr = (u32)p;

    if (addr >= OS_HEAP_AREA_BEGIN && addr <= OS_HEAP_AREA_END)
        return KYGX_MEM_HEAP;

    if (addr >= OS_FCRAM_VADDR && addr <= (OS_FCRAM_VADDR + OS_FCRAM_SIZE))
        return KYGX_MEM_LINEAR;

    if (addr >= OS_VRAM_VADDR && addr <= (OS_VRAM_VADDR + OS_VRAM_SIZE))
        return KYGX_MEM_VRAM;

    if (addr >= OS_QTMRAM_VADDR && addr <= (OS_QTMRAM_VADDR + OS_QTMRAM_SIZE))
        return KYGX_MEM_QTMRAM;

    return KYGX_MEM_UNKNOWN;
}

size_t kygxGetAllocSize(const void* p) {
    switch (kygxGetMemType(p)) {
        case KYGX_MEM_HEAP:
            return malloc_usable_size((void*)p);
        case KYGX_MEM_LINEAR:
            return linearGetSize((void*)p);
        case KYGX_MEM_VRAM:
            return vramGetSize((void*)p);
        case KYGX_MEM_QTMRAM:
            return qtmramGetSize(p);
        default:
            return 0;
    }
}