#include <types.h>
#include <mem_map.h>
#include <arm11/allocator/vram.h>

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
                // TODO
                KYGX_UNREACHABLE("Unimplemented!");
                return NULL;
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
            // TODO
            KYGX_UNREACHABLE("Unimplemented!");
            return NULL;
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
            // TODO
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

    // TODO: check this.
    if (addr >= AXI_RAM_BASE && addr <= A11_HEAP_END)
        return KYGX_MEM_HEAP;

    if (addr >= FCRAM_BASE && addr <= (FCRAM_BASE + FCRAM_SIZE + FCRAM_EXT_SIZE))
        return KYGX_MEM_LINEAR;

    if (addr >= VRAM_BASE && addr <= (VRAM_BASE + VRAM_SIZE))
        return KYGX_MEM_VRAM;

    if (addr >= QTM_RAM_BASE && addr <= (QTM_RAM_BASE + QTM_RAM_SIZE))
        return KYGX_MEM_QTMRAM;

    return KYGX_MEM_UNKNOWN;
}

size_t kygxGetAllocSize(const void* p) {
    switch (kygxGetMemType(p)) {
        case KYGX_MEM_HEAP:
            return malloc_usable_size((void*)p);
        case KYGX_MEM_LINEAR:
            // TODO
            return 0;
        case KYGX_MEM_VRAM:
            return vramGetSize((void*)p);
        case KYGX_MEM_QTMRAM:
            return qtmramGetSize(p);
        default:
            return 0;
    }
}