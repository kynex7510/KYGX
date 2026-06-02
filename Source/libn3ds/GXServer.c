/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <util.h>
#include <kevent.h>
#include <drivers/gfx.h>
#include <drivers/cache.h>
#include <arm11/drivers/gx.h>
#include <arm11/drivers/gpu_regs.h>
#include <arm11/drivers/interrupt.h>

#include <CTR/Assert.h>
#include <CTR/Unreachable.h>

#include "GXServer.h"

#include <string.h>

static GXOnInterrupt g_UserOnInterrupt = NULL;
static GXOnBatchCompleted g_UserOnBatchCompleted = NULL;

static u8 g_NumPendingCommands = 0;
static bool g_FlushWorkaround = false;

static void onInterrupt(KYGXIntr intrID) {
    if (CTR_LIKELY(g_UserOnInterrupt))
        g_UserOnInterrupt(intrID);

    // We use PDC to handle batches with only flush commands.
    if (CTR_LIKELY(intrID == KYGXIntr_PDC0 || intrID == KYGXIntr_PDC1)) {
        if (CTR_LIKELY(!g_FlushWorkaround))
            return;

        g_NumPendingCommands = 1;
        g_FlushWorkaround = false;
    }

    // We should not be getting spurious interrupts.
    CTR_ASSERT(g_NumPendingCommands > 0);

    // Update state.
    --g_NumPendingCommands;

    // Handle batch termination.
    if (--g_NumPendingCommands && CTR_LIKELY(g_UserOnBatchCompleted))
        g_UserOnBatchCompleted();
}

// INTR HACK BEGIN

/*
    Currently libn3ds provides no way of setting a callback for GPU interrupts (akin to gspSetEventCallback in libctru).
    The workaround is to overwrite each interrupt callback with our own. This breaks the gfx api, thus that can't be used
    until kygx is finalized.
*/

#define NUM_INTRS 6

#define INTR_PDC0 0
#define INTR_PDC1 1
#define INTR_PSC0 2
#define INTR_PSC1 3
#define INTR_PPF 4
#define INTR_P3D 5

static KHandle g_AnyEvent;
static u8 g_IntrFlags[NUM_INTRS];

// Used to merge PSC interrupts together.
static bool g_HasPSC0 = false;
static bool g_HasPSC1 = false;

static bool g_PauseIntrHandling = false;
static bool g_ExitThread = false;
static bool g_ExitedThread = false;

static inline void setIntr(size_t index, u8 v) {
    CTR_ASSERT(index < NUM_INTRS);

    u8* p = &g_IntrFlags[index];

    do {
        __ldrexb(p);
    } while (__strexb(p, v));
}

static void intrThread(void* unused) {
    while (true) {
        waitForEvent(g_AnyEvent);
        if (g_ExitThread)
            break;

        if (g_PauseIntrHandling)
            continue;

        u8 intrFlags[NUM_INTRS];
        memcpy(intrFlags, g_IntrFlags, sizeof(g_IntrFlags));

        for (size_t i = 0; i < NUM_INTRS; ++i)
            setIntr(i, 0);

        if (CTR_LIKELY(intrFlags[INTR_PDC0]))
            onInterrupt(KYGXIntr_PDC0);

        if (CTR_LIKELY(intrFlags[INTR_PDC1]))
            onInterrupt(KYGXIntr_PDC1);

        if (intrFlags[INTR_PSC0]) {
            g_HasPSC0 = false;
            if (!g_HasPSC1)
                onInterrupt(KYGXIntr_PSC);
        }

        if (intrFlags[INTR_PSC1]) {
            g_HasPSC1 = false;
            if (!g_HasPSC0)
                onInterrupt(KYGXIntr_PSC);
        }

        if (intrFlags[INTR_PPF])
            g_UserOnInterrupt(KYGXIntr_PPF);

        if (intrFlags[INTR_P3D])
            g_UserOnInterrupt(KYGXIntr_P3D);
    }

    g_ExitedThread = true;
    taskExit();
}

// Keep this as small as possible.
static void intrHandler(uint32_t isr) {
    setIntr(isr - 32 - IRQ_PDC0, 1);
    signalEvent(g_AnyEvent, false);
}

static inline void intrInit(void) {
    g_ExitThread = false;
    g_ExitedThread = false;

    memset(g_IntrFlags, 0, sizeof(g_IntrFlags));

    g_AnyEvent = createEvent(true);
    CTR_BREAK_IF(!g_AnyEvent);

    CTR_BREAK_IF(createTask(32, 3, intrThread, NULL));

    IRQ_registerIsr(IRQ_PDC0, 14, 0, intrHandler);
    IRQ_registerIsr(IRQ_PDC1, 14, 0, intrHandler);
    IRQ_registerIsr(IRQ_PSC0, 14, 0, intrHandler);
    IRQ_registerIsr(IRQ_PSC1, 14, 0, intrHandler);
    IRQ_registerIsr(IRQ_PPF, 14, 0, intrHandler);
    IRQ_registerIsr(IRQ_P3D, 14, 0, intrHandler);
}

static inline void intrExit(void) {
    g_ExitThread = true;
    signalEvent(g_AnyEvent, false);

    IRQ_unregisterIsr(IRQ_PDC0);
    IRQ_unregisterIsr(IRQ_PDC1);
    IRQ_unregisterIsr(IRQ_PSC0);
    IRQ_unregisterIsr(IRQ_PSC1);
    IRQ_unregisterIsr(IRQ_PPF);
    IRQ_unregisterIsr(IRQ_P3D);

    while (!g_ExitedThread)
        yieldTask();

    deleteEvent(g_AnyEvent);

    // Attempt to fix gfx api.
    const GfxFmt topFmt = GFX_getFormat(GFX_LCD_TOP);
    const GfxFmt botFmt = GFX_getFormat(GFX_LCD_BOT);
    const GfxTopMode topMode = GFX_getTopMode();

    GFX_deinit();
    GFX_init(topFmt, botFmt, topMode);
}

// INTR HACK END

void GXServerInit(void) {
    g_UserOnInterrupt = NULL;
    g_UserOnBatchCompleted = NULL;
    g_NumPendingCommands = 0;

    intrInit();
}

void GXServerExit(void) { intrExit(); }

void GXServerSetCallbacks(GXOnInterrupt onInterrupt, GXOnBatchCompleted onBatchCompleted) {
    g_UserOnInterrupt = onInterrupt;
    g_UserOnBatchCompleted = onBatchCompleted;
}

static inline void doProcessCommandList(u32 addr, u32 size, bool updateGasAccMax, bool flush) {
    // TODO: gas

    if (flush)
        flushDCacheRange((void*)addr, size);

    GxRegs* regs = getGxRegs();
    regs->p3d[GPUREG_IRQ_ACK] = 0;

    while (regs->psc_irq_stat & IRQ_STAT_P3D)
        wait_cycles(16);

    regs->p3d[GPUREG_CMDBUF_SIZE0] = size >> 3;
    regs->p3d[GPUREG_CMDBUF_ADDR0] = addr >> 3;
    regs->p3d[GPUREG_CMDBUF_JUMP0] = 1;
}

static inline void doMemoryFill(u32 buf0s, u32 buf0v, u32 buf0e, u32 buf1s, u32 buf1v, u32 buf1e, u32 ctl) {
    GxRegs* regs = getGxRegs();

    while (g_HasPSC0 || g_HasPSC1)
        yieldTask();

    while (!(regs->psc_irq_stat & IRQ_STAT_PSC0) || !(regs->psc_irq_stat & IRQ_STAT_PSC1))
        wait_cycles(16);

    if (buf0s)
        g_HasPSC0 = true;

    if (buf1s)
        g_HasPSC1 = true;

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

    while (!(regs->psc_irq_stat & IRQ_STAT_PPF))
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

    while (!(regs->psc_irq_stat & IRQ_STAT_PPF))
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

GXExecState GXServerExec(CmdIterator* it) {
    CTR_ASSERT(it);

    // Check if we can add commands.
    size_t numCommands = CmdIteratorCount(it);

    if (!numCommands)
        return GXExecState_NoCommands;

    if (g_NumPendingCommands > 0 || g_FlushWorkaround)
        return GXExecState_Busy;

    // Execute commands.
    size_t numFlushCommands = 0;
    g_PauseIntrHandling = true;

    for (size_t i = 0; i < numCommands; ++i) {
        const KYGXCmd* cmd = CmdIteratorNext(it);
        CTR_ASSERT(cmd);

        switch (cmd->header & 0xFF) {
            case KYGX_CMD_REQUESTDMA:
                // TODO
                CTR_UNREACHABLE("Unimplemented!");
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
                ++numFlushCommands;
                break;
            default:
                CTR_UNREACHABLE("Unknown command ID %u", cmd->header & 0xFF);
                break;
        }
    }

    g_NumPendingCommands = numCommands - numFlushCommands;

    // If we have no interrupts, (ab)use PDC.
    if (!g_NumPendingCommands)
        g_FlushWorkaround = true;

    g_PauseIntrHandling = false;
    return GXExecState_Success;
}