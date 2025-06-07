#ifndef _KYGX_UTILITY_H
#define _KYGX_UTILITY_H

#include <GX/Defs.h>

KYGX_INLINE bool kygxIsPo2(u32 v) { return !(v & (v - 1)); }

KYGX_INLINE bool kygxIsAligned(u32 v, u32 alignment) {
    KYGX_ASSERT(kygxIsPo2(alignment));
    return !(v & (alignment - 1));
}

KYGX_INLINE u32 kygxAlignDown(u32 v, u32 alignment) {
    KYGX_ASSERT(kygxIsPo2(alignment));
    return v & ~(alignment - 1);
}

KYGX_INLINE u32 kygxAlignUp(u32 v, u32 alignment) {
    KYGX_ASSERT(kygxIsPo2(alignment));
    return (v + alignment) & ~(alignment - 1);
}

KYGX_EXTERN void kygxInvalidateDataCache(void* addr, size_t size);

#endif /* _KYGX_UTILITY_H */