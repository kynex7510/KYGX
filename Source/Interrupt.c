#include <GX/Interrupt.h>

GXIntr ctrgxPopIntr(GXIntrQueue* q) {
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