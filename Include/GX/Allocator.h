#ifndef _KYGX_ALLOCATOR_H
#define _KYGX_ALLOCATOR_H

#include <GX/Defs.h>

typedef enum {
    KYGX_MEM_HEAP,
    KYGX_MEM_LINEAR,
    KYGX_MEM_VRAM,
    KYGX_MEM_QTMRAM,
    KYGX_MEM_UNKNOWN,
} KYGXMemType;

typedef enum {
    KYGX_ALLOC_VRAM_BANK_A,
    KYGX_ALLOC_VRAM_BANK_B,
    KYGX_ALLOC_VRAM_BANK_ANY,
} KYGXVRAMBank;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void* kygxAllocAligned(KYGXMemType memType, size_t size, size_t alignment);
void* kygxAllocAlignedVRAM(KYGXVRAMBank bank, size_t size, size_t aligment);

KYGX_INLINE void* kygxAlloc(KYGXMemType memType, size_t size) { return kygxAllocAligned(memType, size, 0); }
KYGX_INLINE void* kygxAllocVRAM(KYGXVRAMBank bank, size_t size) { return kygxAllocAlignedVRAM(bank, size, 0); }

void kygxFree(void* p);

KYGXMemType kygxGetMemType(const void* p);
size_t kygxGetAllocSize(const void* p);

KYGX_INLINE bool kygxIsHeap(const void* p) { return kygxGetMemType(p) == KYGX_MEM_HEAP; }
KYGX_INLINE bool kygxIsLinear(const void* p) { return kygxGetMemType(p) == KYGX_MEM_LINEAR; }
KYGX_INLINE bool kygxIsVRAM(const void* p) { return kygxGetMemType(p) == KYGX_MEM_VRAM; }
KYGX_INLINE bool kygxIsQTMRAM(const void* p) { return kygxGetMemType(p) == KYGX_MEM_QTMRAM; }

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _KYGX_ALLOCATOR_H */