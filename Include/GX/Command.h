#ifndef _CTRGX_COMMAND_H
#define _CTRGX_COMMAND_H

#include <GX/Defs.h>

#include <string.h> // memcpy

#define CTRGX_CMDID_REQUESTDMA 0x00
#define CTRGX_CMDID_PROCESSCOMMANDLIST 0x01
#define CTRGX_CMDID_MEMORYFILL 0x02
#define CTRGX_CMDID_DISPLAYTRANSFER 0x03
#define CTRGX_CMDID_TEXTURECOPY 0x04
#define CTRGX_CMDID_FLUSHCACHEREGIONS 0x05

#define CTRGX_CMDHEADER_FLAG_LAST (1 << 16)
#define CTRGX_CMDHEADER_FLAG_FAIL_ON_ALL_BUSY (1 << 24)

#define CTRGX_CMDQUEUE_MAX_COMMANDS 15
#define CTRGX_CMDQUEUE_STATUS_HALTED 0x01
#define CTRGX_CMDQUEUE_STATUS_ERRORED 0x80

typedef struct {
    u32 header;
    u32 params[7];
} GXCmd;

typedef struct {
    u8 index;
    u8 count;
    u8 status;
    u8 requestHalt;
    s32 lastError;
    u8 _pad[0x18];
    GXCmd list[CTRGX_CMDQUEUE_MAX_COMMANDS];
} GXCmdQueue;

CTRGX_INLINE bool ctrgxCmdQueueAdd(GXCmdQueue* q, GXCmd* cmd) {
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

CTRGX_INLINE bool ctrgxCmdQueuePop(GXCmdQueue* q, GXCmd* cmd) {
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

CTRGX_INLINE void ctrgxCmdQueueClearCommands(GXCmdQueue* q) {
    CTRGX_ASSERT(q);

    do {
        u16 v = __ldrexh((u16*)q);
        if (!v) {
            __clrex();
            break;
        }
    } while (__strexh((u16*)q, 0));
}

CTRGX_INLINE void ctrgxCmdQueueSetHalt(GXCmdQueue* q) {
    CTRGX_ASSERT(q);

    do {
        u8 v = __ldrexb(&q->status);
        if (v == CTRGX_CMDQUEUE_STATUS_HALTED) {
            __clrex();
            break;
        }
    } while (__strexb(&q->status, CTRGX_CMDQUEUE_STATUS_HALTED));
}

CTRGX_INLINE void ctrgxCmdQueueClearHalt(GXCmdQueue* q) {
    CTRGX_ASSERT(q);

    do {
        u16 v = __ldrexh((u16*)&q->status);
        if (!v) {
            __clrex();
            break;
        }
    } while (__strexh((u16*)&q->status, 0));
}

CTRGX_INLINE void ctrgxCmdQueueWaitHalt(GXCmdQueue* q) {
    CTRGX_ASSERT(q);

    while (q->status != CTRGX_CMDQUEUE_STATUS_HALTED)
        CTRGX_YIELD();
}

CTRGX_INLINE s32 ctrgxCmdQueueClearError(GXCmdQueue* q) {
    CTRGX_ASSERT(q);

    s32 err;

    do {
        err = __ldrex(CTRGX_EXMON_CAST(&q->lastError));
        if (!err) {
            __clrex();
            break;
        }
    } while (__strex(CTRGX_EXMON_CAST(&q->lastError), 0));

    do {
        u8 status = __ldrexb(&q->status);
        if (status != CTRGX_CMDQUEUE_STATUS_ERRORED) {
            __clrex();
            break;
        }
    } while (__strexb(&q->status, 0));

    return err;
}

#endif /* _CTRGX_COMMAND_H */