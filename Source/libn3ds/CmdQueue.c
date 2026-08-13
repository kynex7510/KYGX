/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <util.h>
#include <kernel.h>
#include <drivers/cache.h>
#include <arm11/drivers/gx.h>
#include <arm11/drivers/gpu_regs.h>

#include <CTR11/Assert.h>
#include <CTR11/Unreachable.h>
#include <CTR11/Sync.h>

#include "CmdQueue.h"
#include "Interrupt.h"

#include <string.h>

typedef struct {
    KYGXCmd cmds[CMDQUEUE_MAX_CMDS];
    size_t count;
} Batch;

static Batch g_Batch;
static Mutex g_BatchMtx;
static CV g_BatchCV;
static bool g_TerminateWorker = false;
static bool g_ExitedThread = false; // Join workaround.

static inline void doRequestDMA(u32 src, u32 dst, u32 size, bool flushSrc) {
    // TODO
    CTR_UNREACHABLE("Unimplemented");
}

static inline void doProcessCommandList(u32 addr, u32 size, bool updateGasAccMax, bool flush) {
    if (flush)
        flushDCacheRange((void*)addr, size);

    GxRegs* regs = getGxRegs();
    regs->p3d[GPUREG_IRQ_ACK] = 0;

    while (regs->psc_irq_stat & IRQ_STAT_P3D)
        wait_cycles(16);

    IntrUpdateGasAccMax(updateGasAccMax);

    regs->p3d[GPUREG_CMDBUF_SIZE0] = size >> 3;
    regs->p3d[GPUREG_CMDBUF_ADDR0] = addr >> 3;
    regs->p3d[GPUREG_CMDBUF_JUMP0] = 1;
}

static inline void doMemoryFill(u32 buf0s, u32 buf0v, u32 buf0e, u32 buf1s, u32 buf1v, u32 buf1e, u32 ctl) {
    GxRegs* regs = getGxRegs();

    while ((regs->psc_irq_stat & IRQ_STAT_PSC0) || (regs->psc_irq_stat & IRQ_STAT_PSC1))
        wait_cycles(16);

    IntrSetPSC(buf0s, buf1s);

    if (buf0s) {
        regs->psc_fill0.s_addr = buf0s >> 3;
        regs->psc_fill0.e_addr = buf0e >> 3;
        regs->psc_fill0.val = buf0v;
        regs->psc_fill0.cnt = (regs->psc_fill0.cnt & 0xFFFF0000) | (ctl & 0xFFFF);
    }

    if (buf1s) {
        regs->psc_fill1.s_addr = buf1s >> 3;
        regs->psc_fill1.e_addr = buf1e >> 3;
        regs->psc_fill1.val = buf1v;
        regs->psc_fill1.cnt = (regs->psc_fill1.cnt & 0xFFFF0000) | (ctl >> 16);
    }
}

static inline void doDisplayTransfer(u32 src, u32 dst, u32 srcDim, u32 dstDim, u32 flags) {
    GxRegs* regs = getGxRegs();

    while (regs->psc_irq_stat & IRQ_STAT_PPF)
        wait_cycles(16);

    regs->ppf.in_addr = src >> 3;
    regs->ppf.out_addr = dst >> 3;
    regs->ppf.dt_outdim = dstDim;
    regs->ppf.dt_indim = srcDim;
    regs->ppf.flags = flags;
    regs->ppf.unk14 = 0;
    regs->ppf.cnt = PPF_EN;
}

static inline void doTextureCopy(u32 src, u32 dst, u32 size, u32 srcParam, u32 dstParam, u32 flags) {
    GxRegs* regs = getGxRegs();

    while (regs->psc_irq_stat & IRQ_STAT_PPF)
        wait_cycles(16);

    regs->ppf.in_addr = src >> 3;
    regs->ppf.out_addr = dst >> 3;
    regs->ppf.len = size;
    regs->ppf.tc_indim = srcParam;
    regs->ppf.tc_outdim = dstParam;
    regs->ppf.flags = flags;
    regs->ppf.cnt = PPF_EN;
}

static inline void doFlushCacheRegions(u32 addr0, u32 size0, u32 addr1, u32 size1, u32 addr2, u32 size2) {
    flushDCacheRange((void*)addr0, size0);

    if (size1) {
        flushDCacheRange((void*)addr1, size1);

        if (size2)
            flushDCacheRange((void*)addr2, size2);
    }
}

static inline void executeCmd(const KYGXCmd* cmd) {
    CTR_ASSERT(cmd);

    switch (cmd->header & 0xFF) {
        case KYGX_CMD_REQUESTDMA:
            doRequestDMA(cmd->params[0], cmd->params[1], cmd->params[2], cmd->params[6] == 1);
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

static void workerThread(void* unused) {
    while (true) {
        AcquireMutex(g_BatchMtx);

        while (!batchSize(&g_Batch) && !g_TerminateWorker)
            WaitCV(g_BatchCV, g_BatchMtx);

        if (g_TerminateWorker) {
            ReleaseMutex(g_BatchMtx);
            break;
        }

        // Copy batch.
        Batch b;
        memcpy(&b, &g_Batch, sizeof(Batch));
        
        g_Batch.count = 0;

        ReleaseMutex(g_BatchMtx);

        // Execute commands.
        const size_t numCmds = batchSize(&b);

        for (size_t i = 0; i < numCmds; ++i)
            executeCmd(&b.cmds[i]);
    }

    //
    g_ExitedThread = true;
    //
    
    taskExit();
}

void CmdQueueInit(void) {
    CTR_ASSERT(!g_BatchMtx);

    g_Batch.count = 0;
    g_TerminateWorker = false;

    g_BatchMtx = CreateMutex();
    g_BatchCV = CreateCV();

    CTR_BREAK_IF(!createTask(0x400, 3, workerThread, NULL));
}

void CmdQueueExit(void) {
    CTR_ASSERT(g_BatchMtx);

    AcquireMutex(g_BatchMtx);
    g_ExitedThread = false;
    g_TerminateWorker = true;
    NotifyCV(g_BatchCV, 1);
    ReleaseMutex(g_BatchMtx);

    //
    while (!g_ExitedThread)
        yieldTask();
    //

    DestroyCV(g_BatchCV);
    DestroyMutex(g_BatchMtx);

    g_BatchCV = NULL;
    g_BatchMtx = NULL;
}

bool CmdQueueIsBusy(void) {
    CTR_ASSERT(g_BatchMtx);

    // Batch commands are all added in one go, thus the queue becomes busy as soon as we get one.
    AcquireMutex(g_BatchMtx);
    const size_t num = batchSize(&g_Batch);
    ReleaseMutex(g_BatchMtx);
    return num > 0;
}

void CmdQueueAdd(const KYGXCmd* cmd) {
    CTR_ASSERT(g_BatchMtx);

    AcquireMutex(g_BatchMtx);

    CTR_ASSERT((batchSize(&g_Batch) + 1) < CMDQUEUE_MAX_CMDS);

    memcpy(&g_Batch.cmds[g_Batch.count], cmd, sizeof(KYGXCmd));
    ++g_Batch.count;

    ReleaseMutex(g_BatchMtx);
}

void CmdQueueTriggerHandling(void) {
    CTR_ASSERT(g_BatchMtx);
    NotifyCV(g_BatchCV, 1);
}