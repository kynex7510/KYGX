/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _KYGX_INTERRUPT_H
#define _KYGX_INTERRUPT_H

#include <KYGX/Defs.h>

#define KYGX_INTRQUEUE_MAX_INTERRUPTS 52
#define KYGX_INTRQUEUE_MAX_PDC_INTERRUPTS 32
#define KYGX_INTRQUEUE_FLAG_SKIP_PDC 0x01

#define KYGX_NUM_INTERRUPTS 7

typedef enum {
    KYGX_INTR_PSC0 = 0,
    KYGX_INTR_PSC1 = 1,
    KYGX_INTR_PDC0 = 2,
    KYGX_INTR_PDC1 = 3,
    KYGX_INTR_PPF = 4,
    KYGX_INTR_P3D = 5,
    KYGX_INTR_DMA = 6,
    KYGX_INTR_UNKNOWN = -1,
} KYGXIntr;

typedef struct KYGX_PACKED {
    u8 index;
    u8 count;
    u8 missedOthers;
    u8 flags;
    u32 missedPDC0;
    u32 missedPDC1;
    u8 list[KYGX_INTRQUEUE_MAX_INTERRUPTS];
} KYGXIntrQueue;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

KYGX_INLINE KYGXIntr kygxPopIntr(KYGXIntrQueue* q) {
    KYGX_ASSERT(q);

    KYGXIntr intr = KYGX_INTR_UNKNOWN;
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

KYGX_INLINE void kygxIntrQueueSkipPDC(KYGXIntrQueue* q, bool skip) {
    KYGX_ASSERT(q);

    u8 f;
    do {
        f = __ldrexb(&q->flags);
    } while (__strexb(&q->flags, skip ? (f | KYGX_INTRQUEUE_FLAG_SKIP_PDC) : (f & ~KYGX_INTRQUEUE_FLAG_SKIP_PDC)));
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _KYGX_INTERRUPT_H */