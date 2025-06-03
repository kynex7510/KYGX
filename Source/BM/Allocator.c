#include <mem_map.h>
#include <arm11/allocator/vram.h>

#include <GX/Allocator.h>

#include <stdlib.h> // malloc, free
#include <malloc.h> // malloc_usable_size

void* ctrgxAllocAligned(GXMemType memType, size_t size, size_t alignment) {
    if (alignment == GX_ALLOC_ALIGN_DEFAULT) {
        switch (memType) {
            case GX_MEM_HEAP:
                return malloc(size);
            case GX_MEM_LINEAR:
                // TODO
                CTRGX_UNREACHABLE("Unimplemented!");
                break;
            case GX_MEM_VRAM:
                return vramAlloc(size);
            case GX_MEM_QTM:
                // TODO
                CTRGX_UNREACHABLE("Unimplemented!");
                break;
            default:
                return NULL;
        }
    }

    switch (memType) {
        case GX_MEM_HEAP:
            return memalign(alignment, size);
        case GX_MEM_LINEAR:
            // TODO
            CTRGX_UNREACHABLE("Unimplemented!");
            break;
        case GX_MEM_VRAM:
            return vramMemAlign(size, alignment);
        case GX_MEM_QTM:
            // TODO
            CTRGX_UNREACHABLE("Unimplemented!");
            break;
        default:
            return NULL;
    }
}

CTRGX_INLINE vramAllocPos getVRAMPos(GXVRAMBank bank) {
    if (bank == GX_ALLOC_VRAM_BANK_A)
        return VRAM_ALLOC_A;

    if (bank == GX_ALLOC_VRAM_BANK_B)
        return VRAM_ALLOC_B;

    return VRAM_ALLOC_ANY;
}

void* ctrgxAllocAlignedVRAM(GXVRAMBank bank, size_t size, size_t aligment) {
    if (aligment == GX_ALLOC_ALIGN_DEFAULT)
        return vramAllocAt(size, getVRAMPos(bank));
    
    return vramMemAlignAt(size, aligment, getVRAMPos(bank));
}

void ctrgxFree(void* p) {
    switch (ctrgxGetAllocType(p)) {
        case GX_MEM_HEAP:
            free(p);
            break;
        case GX_MEM_LINEAR:
            // TODO
            break;
        case GX_MEM_VRAM:
            vramFree(p);
            break;
        case GX_MEM_QTM:
            // TODO
            break;
        default:;
    }
}

GXMemType ctrgxGetMemType(const void* p) {
    const u32 addr = (u32)p;

    if (addr >= AXI_RAM_BASE && addr <= A11_HEAP_END)
        return GX_MEM_HEAP;

    if (addr >= FCRAM_BASE && addr <= (FCRAM_BASE + FCRAM_EXT_SIZE))
        return GX_MEM_LINEAR;

    if (addr >= VRAM_BASE && addr <= (VRAM_BASE + VRAM_SIZE))
        return GX_MEM_VRAM;

    if (addr >= QTM_RAM_BASE && addr <= (QTM_RAM_BASE + QTM_RAM_SIZE))
        return GX_MEM_QTM;

    return GX_MEM_UNKNOWN;
}