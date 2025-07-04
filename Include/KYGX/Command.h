/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _KYGX_COMMAND_H
#define _KYGX_COMMAND_H

#include <KYGX/Defs.h>

#include <string.h> // memcpy

#define KYGX_CMDID_REQUESTDMA 0x00
#define KYGX_CMDID_PROCESSCOMMANDLIST 0x01
#define KYGX_CMDID_MEMORYFILL 0x02
#define KYGX_CMDID_DISPLAYTRANSFER 0x03
#define KYGX_CMDID_TEXTURECOPY 0x04
#define KYGX_CMDID_FLUSHCACHEREGIONS 0x05

#define KYGX_CMDHEADER_FLAG_LAST (1 << 16)
#define KYGX_CMDHEADER_FLAG_FAIL_ON_ALL_BUSY (1 << 24)

#define KYGX_CMDQUEUE_MAX_COMMANDS 15
#define KYGX_CMDQUEUE_STATUS_HALTED 0x01
#define KYGX_CMDQUEUE_STATUS_ERRORED 0x80

typedef struct {
    u32 header;
    u32 params[7];
} KYGXCmd;

typedef struct {
    u8 index;
    u8 count;
    u8 status;
    u8 requestHalt;
    s32 lastError;
    u8 _pad[0x18];
    KYGXCmd list[KYGX_CMDQUEUE_MAX_COMMANDS];
} KYGXCmdQueue;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

KYGX_INLINE bool kygxCmdQueueAdd(KYGXCmdQueue* q, const KYGXCmd* cmd) {
    KYGX_ASSERT(q);
    KYGX_ASSERT(cmd);

    u32 header;
    
    do {
        header = __ldrex(KYGX_EXMON_CAST(q));
        const u8 count = (header >> 8) & 0xFF;
        if (count >= KYGX_CMDQUEUE_MAX_COMMANDS) {
            __clrex();
            return false;
        }

        const u8 index = (count + (header & 0xFF)) % KYGX_CMDQUEUE_MAX_COMMANDS;
        memcpy(&q->list[index], cmd, sizeof(KYGXCmd));
        __dsb();

        header = (header & 0xFFFF00FF) | ((u16)(count + 1) << 8);
    } while (__strex(KYGX_EXMON_CAST(q), header));

    return true;
}

KYGX_INLINE bool kygxCmdQueuePop(KYGXCmdQueue* q, KYGXCmd* cmd) {
    KYGX_ASSERT(q);
    KYGX_ASSERT(cmd);

    u32 header;

    do {
        header = __ldrex(KYGX_EXMON_CAST(q));
        const u8 count = (header >> 8) & 0xFF;
        if (!count) {
            __clrex();
            return false;
        }

        const u8 index = (header & 0xFF) % KYGX_CMDQUEUE_MAX_COMMANDS;
        memcpy(cmd, &q->list[index], sizeof(KYGXCmd));

        header = (header & 0xFFFF0000) | ((u16)(count - 1) << 8) | (index + 1);
    } while (__strex(KYGX_EXMON_CAST(q), header));

    return true;
}

KYGX_INLINE void kygxCmdQueueClearCommands(KYGXCmdQueue* q) {
    KYGX_ASSERT(q);

    do {
        u16 v = __ldrexh((u16*)q);
        if (!v) {
            __clrex();
            break;
        }
    } while (__strexh((u16*)q, 0));
}

KYGX_INLINE void kygxCmdQueueSetHalt(KYGXCmdQueue* q) {
    KYGX_ASSERT(q);

    do {
        u8 v = __ldrexb(&q->status);
        if (v == KYGX_CMDQUEUE_STATUS_HALTED) {
            __clrex();
            break;
        }
    } while (__strexb(&q->status, KYGX_CMDQUEUE_STATUS_HALTED));
}

KYGX_INLINE void kygxCmdQueueClearHalt(KYGXCmdQueue* q) {
    KYGX_ASSERT(q);

    do {
        u16 v = __ldrexh((u16*)&q->status);
        if (!v) {
            __clrex();
            break;
        }
    } while (__strexh((u16*)&q->status, 0));
}

KYGX_INLINE void kygxCmdQueueWaitHalt(KYGXCmdQueue* q) {
    KYGX_ASSERT(q);

    while (q->status != KYGX_CMDQUEUE_STATUS_HALTED)
        KYGX_YIELD();
}

KYGX_INLINE s32 kygxCmdQueueClearError(KYGXCmdQueue* q) {
    KYGX_ASSERT(q);

    s32 err;

    do {
        err = __ldrex(KYGX_EXMON_CAST(&q->lastError));
        if (!err) {
            __clrex();
            break;
        }
    } while (__strex(KYGX_EXMON_CAST(&q->lastError), 0));

    do {
        u8 status = __ldrexb(&q->status);
        if (status != KYGX_CMDQUEUE_STATUS_ERRORED) {
            __clrex();
            break;
        }
    } while (__strexb(&q->status, 0));

    return err;
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _KYGX_COMMAND_H */