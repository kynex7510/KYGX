/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <3ds.h>

#include <CTR11/Assert.h>
#include <CTR11/Break.h>
#include <CTR11/Memory.h>
#include <CTR11/Unreachable.h>
#include <CTR11/Cache.h>

#include "CmdQueue.h"
#include "Interrupt.h"

#include <string.h>

#define GSP_OFFSET(addr) ((addr) - 0x1EB00000)

typedef struct {
    KYGXCmd cmds[CMDQUEUE_MAX_CMDS];
    size_t count;
} Batch;

static Batch g_Batch;

static inline void doRequestDMA(const KYGXCmd* cmd) {
    // It makes sense to rely on GSP for this one as GSP has VRAM always mapped.
    gspSubmitGxCommand((u32*)cmd);
}

static inline void doProcessCommandList(u32 addr, u32 size, bool updateGasAccMax, bool flush) {
    if (flush)
        FlushDataCache((void*)addr, size);
    
    const u32 buffer[5] = {
        size >> 3,
        0, // padding
        osConvertVirtToPhys((void*)addr) >> 3,
        0, // padding
        1, // cnt
    };

    // TODO: needs changes in Interrupt.c.
    IntrUpdateGasAccMax(false);

    CTR_BREAK_IF(R_FAILED(GSPGPU_WriteHWRegs(GSP_OFFSET(0x1EF018E0), buffer, sizeof(buffer))));
}

static inline void doMemoryFill(u32 buf0s, u32 buf0v, u32 buf0e, u32 buf1s, u32 buf1v, u32 buf1e, u32 ctl) {
    const u32 buffer[8] = {
        osConvertVirtToPhys((void*)buf0s) >> 3,
        buf0s ? osConvertVirtToPhys((void*)buf0e) >> 3 : 0,
        buf0v,
        buf0s ? (ctl & 0xFFFF) : 0,
        osConvertVirtToPhys((void*)buf1s) >> 3,
        buf1s ? osConvertVirtToPhys((void*)buf1e) >> 3 : 0,
        buf1v,
        buf1s ? (ctl >> 16) : 0,
    };

    CTR_BREAK_IF(R_FAILED(GSPGPU_WriteHWRegs(GSP_OFFSET(0x1EF00010), buffer, sizeof(buffer))));
}

static inline void doDisplayTransfer(u32 src, u32 dst, u32 srcDim, u32 dstDim, u32 flags) {
    const u32 buffer[7] = {
        osConvertVirtToPhys((void*)src) >> 3,
        osConvertVirtToPhys((void*)dst) >> 3,
        dstDim,
        srcDim,
        flags,
        0, // unk
        1, // cnt
    };

    CTR_BREAK_IF(R_FAILED(GSPGPU_WriteHWRegs(GSP_OFFSET(0x1EF00C00), buffer, sizeof(buffer))));
}

static inline void doTextureCopy(u32 src, u32 dst, u32 size, u32 srcParam, u32 dstParam, u32 flags) {
    const u32 buffer[3] = {
        size,
        srcParam,
        dstParam,
    };

    CTR_BREAK_IF(R_FAILED(GSPGPU_WriteHWRegs(GSP_OFFSET(0x1EF00C20), buffer, sizeof(buffer))));
    doDisplayTransfer(src, dst, 0, 0, flags);
}

static inline void doFlushCacheRegions(u32 addr0, u32 size0, u32 addr1, u32 size1, u32 addr2, u32 size2) {
    FlushDataCache((void*)addr0, size0);

    if (size1) {
        FlushDataCache((void*)addr1, size1);

        if (size2)
            FlushDataCache((void*)addr2, size2);
    }
}

static inline void executeCmd(const KYGXCmd* cmd) {
    CTR_ASSERT(cmd);

    switch (cmd->header & 0xFF) {
        case KYGX_CMD_REQUESTDMA:
            doRequestDMA(cmd);
            break;
        case KYGX_CMD_PROCESSCOMMANDLIST:
            doProcessCommandList(cmd->params[0], cmd->params[1], cmd->params[2] == 1, cmd->params[6] == 1);
            break;
        case KYGX_CMD_MEMORYFILL:
            doMemoryFill(cmd->params[0], cmd->params[1], cmd->params[2], cmd->params[3], cmd->params[4], cmd->params[5], cmd->params[6]);
            break;
        case KYGX_CMD_DISPLAYTRANSFER:
            doDisplayTransfer(cmd->params[0], cmd->params[1], cmd->params[2], cmd->params[3], cmd->params[4]);
            break;
        case KYGX_CMD_TEXTURECOPY:
            doTextureCopy(cmd->params[0], cmd->params[1], cmd->params[2], cmd->params[3], cmd->params[4], cmd->params[5]);
            break;
        case KYGX_CMD_FLUSHCACHEREGIONS:
            doFlushCacheRegions(cmd->params[0], cmd->params[1], cmd->params[2], cmd->params[3], cmd->params[4], cmd->params[5]);
            break;
        default:
            CTR_UNREACHABLE("Unknown command ID %u", cmd->header & 0xFF);
            break;
    }
}

static inline size_t batchSize(const Batch* b) {
    CTR_ASSERT(b);
    return b->count;
}

void CmdQueueInit(void) { g_Batch.count = 0; }
void CmdQueueExit(void) {}
bool CmdQueueIsBusy(void) { return false; }

void CmdQueueAdd(const KYGXCmd* cmd) {
    CTR_ASSERT((batchSize(&g_Batch) + 1) < CMDQUEUE_MAX_CMDS);

    memcpy(&g_Batch.cmds[g_Batch.count], cmd, sizeof(KYGXCmd));
    ++g_Batch.count;
}

void CmdQueueTriggerHandling(void) {
    const size_t numCmds = batchSize(&g_Batch);

    for (size_t i = 0; i < numCmds; ++i) {
        const KYGXCmd* cmd = &g_Batch.cmds[i];
        executeCmd(cmd);

        switch (cmd->header & 0xFF) {
            case KYGX_CMD_DISPLAYTRANSFER:
            case KYGX_CMD_TEXTURECOPY:
                kygxWaitIntr(KYGXIntr_PPF);
                break;
        }
    }

    g_Batch.count = 0;
}