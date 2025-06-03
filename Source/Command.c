#include <GX/Command.h>

#include <string.h> // memcpy

bool ctrgxCmdQueueAdd(GXCmdQueue* q, GXCmd* cmd) {
    CTRGX_ASSERT(q);
    CTRGX_ASSERT(cmd);

    u32 header;
    
    do {
        header = __ldrex(CTRGX_EXMON_CAST(q));
        const u8 count = (header >> 8) & 0xFF;
        if (count >= CTRGX_CMDQUEUE_MAX_COMMANDS) {
            __clrex();
            return false;
        }

        const u8 index = (count + (header & 0xFF)) % CTRGX_CMDQUEUE_MAX_COMMANDS;
        memcpy(&q->list[index], cmd, sizeof(GXCmd));
        __dsb();

        header = (header & 0xFFFF00FF) | ((u16)(count + 1) << 8);
    } while (__strex(CTRGX_EXMON_CAST(q), header));

    return true;
}

bool ctrgxCmdQueuePop(GXCmdQueue* q, GXCmd* cmd) {
    CTRGX_ASSERT(q);
    CTRGX_ASSERT(cmd);

    u32 header;

    do {
        header = __ldrex(CTRGX_EXMON_CAST(q));
        const u8 count = (header >> 8) & 0xFF;
        if (!count) {
            __clrex();
            return false;
        }

        const u8 index = (header & 0xFF) % CTRGX_CMDQUEUE_MAX_COMMANDS;
        memcpy(cmd, &q->list[index], sizeof(GXCmd));

        header = (header & 0xFFFF0000) | ((u16)(count - 1) << 8) | (index + 1);
    } while (__strex(CTRGX_EXMON_CAST(q), header));

    return true;
}