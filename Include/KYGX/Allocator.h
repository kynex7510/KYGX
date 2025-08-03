/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _KYGX_ALLOCATOR_H
#define _KYGX_ALLOCATOR_H

#include <KYGX/Defs.h>

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

// newSize == 0 frees the buffer. VRAM reallocation doesn't retain content.
void* kygxRealloc(void* p, size_t newSize);

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