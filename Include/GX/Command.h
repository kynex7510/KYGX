#ifndef _CTRGX_COMMAND_H
#define _CTRGX_COMMAND_H

#include <GX/Defs.h>

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
    u8 isHalted;
    s32 lastError;
    u8 _pad[0x18];
    GXCmd list[CTRGX_CMDQUEUE_MAX_COMMANDS];
} GXCmdQueue;

CTRGX_EXTERN bool ctrgxCmdQueueAdd(GXCmdQueue* q, GXCmd* cmd);
CTRGX_EXTERN bool ctrgxCmdQueuePop(GXCmdQueue* q, GXCmd* cmd);

CTRGX_INLINE void ctrgxCmdQueueClear(GXCmdQueue* q) {
    CTRGX_ASSERT(q);

    do {
        __ldrexh((u16*)q);
    } while (__strexh((u16*)q, 0));
}

CTRGX_INLINE void ctrgxCmdQueueHalt(GXCmdQueue* q, bool halt) {
    CTRGX_ASSERT(q);

    u32 h;

    do {
        h = __ldrex(CTRGX_EXMON_CAST(q));
        if (halt) {
            h |= 0x01010000;
        } else {
            h &= ~(0x01010000);
        }
    } while (__strex(CTRGX_EXMON_CAST(q), h));
}

CTRGX_INLINE bool ctrgxCmdQueueIsHalted(GXCmdQueue* q) {
    CTRGX_ASSERT(q);
    return q->isHalted || (q->status & CTRGX_CMDQUEUE_STATUS_HALTED);
}

CTRGX_INLINE s32 ctrgxCmdQueueClearError(GXCmdQueue* q) {
    CTRGX_ASSERT(q);

    u8 status;
    s32 err;

    do {
        err = __ldrex(CTRGX_EXMON_CAST(&q->lastError));
    } while (__strex(CTRGX_EXMON_CAST(&q->lastError), 0));

    do {
        status = __ldrexb(&q->status);
    } while (__strexb(&q->status, status & ~CTRGX_CMDQUEUE_STATUS_ERRORED));

    return err;
}

#endif /* _CTRGX_COMMAND_H */