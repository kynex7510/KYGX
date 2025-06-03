#ifndef _CTRGX_ALLOCATOR_H
#define _CTRGX_ALLOCATOR_H

#include <GX/Defs.h>

#define GX_ALLOC_ALIGN_DEFAULT (size_t)(-1)

typedef enum {
    GX_MEM_HEAP,
    GX_MEM_LINEAR,
    GX_MEM_VRAM,
    GX_MEM_QTM,
    GX_MEM_UNKNOWN,
} GXMemType;

typedef enum {
    GX_ALLOC_VRAM_BANK_A,
    GX_ALLOC_VRAM_BANK_B,
    GX_ALLOC_VRAM_BANK_ANY,
} GXVRAMBank;

void* ctrgxAllocAligned(GXMemType memType, size_t size, size_t alignment);
void* ctrgxAllocAlignedVRAM(GXVRAMBank bank, size_t size, size_t aligment);
void ctrgxFree(void* p);
GXMemType ctrgxGetMemType(const void* p);
size_t ctrgxGetAllocSize(const void* p);

CTRGX_INLINE void* ctrgxAlloc(GXMemType memType, size_t size) { return ctrgxAllocAligned(memType, size, GX_ALLOC_ALIGN_DEFAULT); }
CTRGX_INLINE void* ctrgxAllocVRAM(GXVRAMBank bank, size_t size) { return ctrgxAllocAlignedVRAM(bank, size, GX_ALLOC_ALIGN_DEFAULT); }

CTRGX_INLINE bool ctrgxIsHeapMem(const void* p) { return ctrgxGetMemType(p) == GX_MEM_HEAP; }
CTRGX_INLINE bool ctrgxIsLinearMem(const void* p) { return ctrgxGetMemType(p) == GX_MEM_LINEAR; }
CTRGX_INLINE bool ctrgxIsVRAMMem(const void* p) { return ctrgxGetMemType(p) == GX_MEM_VRAM; }
CTRGX_INLINE bool ctrgxIsQTMMem(const void* p) { return ctrgxGetMemType(p) == GX_MEM_QTM; }

#endif /* _CTRGX_ALLOCATOR_H */