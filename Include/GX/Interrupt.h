#ifndef _KYGX_INTERRUPT_H
#define _KYGX_INTERRUPT_H

#include <GX/Defs.h>

#define KYGX_INTRQUEUE_MAX_INTERRUPTS 52
#define KYGX_INTRQUEUE_MAX_PDC_INTERRUPTS 32
#define KYGX_INTRQUEUE_FLAG_SKIP_PDC 0x01

#define KYGX_NUM_INTERRUPTS 7

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

typedef struct KYGX_PACKED {
    u8 index;
    u8 count;
    u8 missedOthers;
    u8 flags;
    u32 missedPDC0;
    u32 missedPDC1;
    u8 list[KYGX_INTRQUEUE_MAX_INTERRUPTS];
} GXIntrQueue;

KYGX_INLINE GXIntr kygxPopIntr(GXIntrQueue* q) {
    KYGX_ASSERT(q);

    GXIntr intr = GX_INTR_UNKNOWN;
    u32 header;

    do {
        header = __ldrex(KYGX_EXMON_CAST(q));

        const u8 count = (header >> 8) & 0xFF;
        if (!count) {
            __clrex();
            break;
        }

        const u8 index = header & 0xFF;
        intr = q->list[(index + count) % KYGX_INTRQUEUE_MAX_INTERRUPTS];

        header = (header & 0xFFFF0000) | ((u16)(count - 1) << 8) | (index + 1);
    } while (__strex(KYGX_EXMON_CAST(q), header));

    return intr;
}

KYGX_INLINE void kygxIntrQueueSkipPDC(GXIntrQueue* q, bool skip) {
    KYGX_ASSERT(q);

    u8 f;
    do {
        f = __ldrexb(&q->flags);
    } while (__strexb(&q->flags, skip ? (f | KYGX_INTRQUEUE_FLAG_SKIP_PDC) : (f & ~KYGX_INTRQUEUE_FLAG_SKIP_PDC)));
}

#endif /* _KYGX_INTERRUPT_H */