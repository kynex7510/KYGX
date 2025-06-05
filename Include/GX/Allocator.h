#ifndef _KYGX_ALLOCATOR_H
#define _KYGX_ALLOCATOR_H

#include <GX/Defs.h>

typedef enum {
    GX_MEM_HEAP,
    GX_MEM_LINEAR,
    GX_MEM_VRAM,
    GX_MEM_QTMRAM,
    GX_MEM_UNKNOWN,
} GXMemType;

typedef enum {
    GX_ALLOC_VRAM_BANK_A,
    GX_ALLOC_VRAM_BANK_B,
    GX_ALLOC_VRAM_BANK_ANY,
} GXVRAMBank;

void* kygxAllocAligned(GXMemType memType, size_t size, size_t alignment);
void* kygxAllocAlignedVRAM(GXVRAMBank bank, size_t size, size_t aligment);
void kygxFree(void* p);
GXMemType kygxGetMemType(const void* p);
size_t kygxGetAllocSize(const void* p);

KYGX_INLINE void* kygxAlloc(GXMemType memType, size_t size) { return kygxAllocAligned(memType, size, 0); }
KYGX_INLINE void* kygxAllocVRAM(GXVRAMBank bank, size_t size) { return kygxAllocAlignedVRAM(bank, size, 0); }

KYGX_INLINE bool kygxIsHeap(const void* p) { return kygxGetMemType(p) == GX_MEM_HEAP; }
KYGX_INLINE bool kygxIsLinear(const void* p) { return kygxGetMemType(p) == GX_MEM_LINEAR; }
KYGX_INLINE bool kygxIsVRAM(const void* p) { return kygxGetMemType(p) == GX_MEM_VRAM; }
KYGX_INLINE bool kygxIsQTMRAM(const void* p) { return kygxGetMemType(p) == GX_MEM_QTMRAM; }

#endif /* _KYGX_ALLOCATOR_H */