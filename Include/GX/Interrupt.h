#ifndef _CTRGX_INTERRUPT_H
#define _CTRGX_INTERRUPT_H

#include <GX/Defs.h>

#define CTRGX_INTRQUEUE_MAX_INTERRUPTS 52
#define CTRGX_INTRQUEUE_MAX_PDC_INTERRUPTS 32
#define CTRGX_INTRQUEUE_FLAG_SKIP_PDC 0x01

#define CTRGX_NUM_INTERRUPTS 7

typedef enum {
    GX_INTR_PSC0 = 0,
    GX_INTR_PSC1 = 1,
    GX_INTR_PDC0 = 2,
    GX_INTR_PDC1 = 3,
    GX_INTR_PPF = 4,
    GX_INTR_P3D = 5,
    GX_INTR_DMA = 6,
    GX_INTR_UNKNOWN = -1,
} GXIntr;

typedef struct CTRGX_PACKED {
    u8 index;
    u8 count;
    u8 missedOthers;
    u8 flags;
    u32 missedPDC0;
    u32 missedPDC1;
    u8 list[CTRGX_INTRQUEUE_MAX_INTERRUPTS];
} GXIntrQueue;

CTRGX_INLINE GXIntr ctrgxPopIntr(GXIntrQueue* q) {
    CTRGX_ASSERT(q);

    GXIntr intr = GX_INTR_UNKNOWN;
    u32 header;

    do {
        header = __ldrex(CTRGX_EXMON_CAST(q));

        const u8 count = (header >> 8) & 0xFF;
        if (!count) {
            __clrex();
            break;
        }

        const u8 index = header & 0xFF;
        intr = q->list[(index + count) % CTRGX_INTRQUEUE_MAX_INTERRUPTS];

        header = (header & 0xFFFF0000) | ((u16)(count - 1) << 8) | (index + 1);
    } while (__strex(CTRGX_EXMON_CAST(q), header));

    return intr;
}

CTRGX_INLINE void ctrgxIntrQueueSkipPDC(GXIntrQueue* q, bool skip) {
    CTRGX_ASSERT(q);

    u8 f;
    do {
        f = __ldrexb(&q->flags);
    } while (__strexb(&q->flags, skip ? (f | CTRGX_INTRQUEUE_FLAG_SKIP_PDC) : (f & ~CTRGX_INTRQUEUE_FLAG_SKIP_PDC)));
}

#endif /* _CTRGX_INTERRUPT_H */