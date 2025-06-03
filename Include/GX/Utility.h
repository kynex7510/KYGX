#ifndef _CTRGX_UTILITY_H
#define _CTRGX_UTILITY_H

#include <GX/Defs.h>

CTRGX_INLINE bool ctrgxIsPo2(u32 v) { return !(v & (v - 1)); }

CTRGX_INLINE bool ctrgxIsAligned(u32 v, u32 alignment) {
    CTRGX_ASSERT(ctrgxIsPo2(alignment));
    return !(v & (alignment - 1));
}

CTRGX_INLINE u32 ctrgxAlignDown(u32 v, u32 alignment) {
    CTRGX_ASSERT(ctrgxIsPo2(alignment));
    return v & ~(alignment - 1);
}

CTRGX_INLINE u32 ctrgxAlignUp(u32 v, u32 alignment) {
    CTRGX_ASSERT(ctrgxIsPo2(alignment));
    return (v + alignment) & ~(alignment - 1);
}

#endif /* _CTRGX_UTILITY_H */