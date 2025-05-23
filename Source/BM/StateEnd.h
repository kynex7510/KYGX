#include "../State.h"

#define SAFE_OPS (STATEOP_INTRCB | STATEOP_INTR_READ | STATEOP_INTR_WRITE)

static GXCmdQueue g_CmdQueue;

static InterruptCallback g_IntrCallbacks[4] = {};
static u8 g_ShouldTerminateThreads = 0;
static u8 g_RunningThreads = 0;

static size_t cbIndexForIntr(GXIntr intrID) {
    switch (intrID) {
        case GX_INTR_PSC0:
            return 0;
        case GX_INTR_PSC1:
            return 1;
        case GX_INTR_PPF:
            return 2;
        case GX_INTR_P3D:
            return 3;
        default:
            CTRGX_UNREACHABLE("Invalid interrupt ID!"); // Shouldn't happen.
    }
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

    if (buf0s) {
        regs->psc_fill0.s_addr = buf0s >> 3;
        regs->psc_fill0.e_addr = buf0e >> 3;
        regs->psc_fill0.val = buf0v;
        regs->psc_fill0.cnt |= (regs->psc_fill0.cnt & 0xFFFF0000) | (ctl & 0xFFFF);
    }

    if (buf1s) {
        regs->psc_fill1.s_addr = buf1s >> 3;
        regs->psc_fill1.e_addr = buf0e >> 3;
        regs->psc_fill1.val = buf1v;
        regs->psc_fill1.cnt != (regs->psc_fill1.cnt & 0xFFFF0000) | (ctl >> 16);
    }
}

static inline void doDisplayTransfer(u32 src, u32 dst, u32 srcDim, u32 dstDim, u32 flags) {
    GxRegs* regs = getGxRegs();

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

    regs->ppf.in_addr = src >> 3;
    regs->ppf.out_addr = dst >> 3;
    regs->ppf.len = size;
    regs->ppf.tc_indim = srcParam;
    regs->ppf.tc_outdim = dstParam;
    regs->ppf.flags = flags;
    regs->ppf.cnt |= PPF_EN;
}

static inline void doFlushCacheRegions(u32 addr0, u32 size0, u32 addr1, u32 size1, u32 addr2, u32 size2) {
    flushDCacheRange((void*)addr0, size0);

    if (size1) {
        flushDCacheRange((void*)addr1, size1);

        if (size2)
            flushDCacheRange((void*)addr2, size2);
    }
}

static void execCommand(const GXCmd* cmd) {
    switch (cmd->header & 0xFF) {
        case CTRGX_CMDID_PROCESSCOMMANDLIST:
            doProcessCommandList((void*)cmd->params[0], cmd->params[1], cmd->params[2] == 1, cmd->params[6] == 1);
            break;
        case CTRGX_CMDID_MEMORYFILL:
            doMemoryFill(cmd->params[0], cmd->params[1], cmd->params[2], cmd->params[3], cmd->params[4], cmd->params[5], cmd->params[6]);
            break;
        case CTRGX_CMDID_DISPLAYTRANSFER:
            doDisplayTransfer(cmd->params[0], cmd->params[1], cmd->params[2], cmd->params[3], cmd->params[4]);
            break;
        case CTRGX_CMDID_TEXTURECOPY:
            doTextureCopy(cmd->params[0], cmd->params[1], cmd->params[2], cmd->params[3], cmd->params[4], cmd->params[5]);
            break;
        case CTRGX_CMDID_FLUSHCACHEREGIONS:
            doFlushCacheRegions(cmd->params[0], cmd->params[1], cmd->params[2], cmd->params[3], cmd->params[4], cmd->params[5]);
            break;
    }
}

static void interruptHandler(GXIntr intrID) {
    const u8 index = cbIndexForIntr(intrID);

    // Signal running.
    u8 v;
    do {
        v = __ldrexb(&g_RunningThreads);
    } while (__strexb(&g_RunningThreads, v | (1 << index)));

    // Process incoming interrupts.
    while (true) {
        GFX_waitForEvent((GfxEvent)intrID);

        if (g_ShouldTerminateThreads)
            break;

        // Invoke callback.
        InterruptCallback cb = g_IntrCallbacks[index];
        if (cb)
            cb(intrID);
    }

    // Signal termination.
    do {
        v = __ldrexb(&g_RunningThreads);
    } while (__strexb(&g_RunningThreads, v & ~(1 << index)));

    taskExit();
}

bool ctrgxs_init(State* state) {
    CTRGX_ASSERT(state);

    do {
        __ldrexb(g_ShouldTerminateThreads);
    } while (__strexb(&g_ShouldTerminateThreads, 0));
    
    do {
        __ldrexb(g_RunningThreads);
    } while (__strexb(&g_RunningThreads, 0));

    state->platform.lock = createMutex();
    CTRGX_ASSERT(state->platform.lock != NULL);

    CV_Init(&state->platform.completionCV);
    CV_Init(&state->platform.haltCV);
    state->platform.haltRequested = false;
    state->platform.halted = true;

    state->intrQueue = NULL;
    state->cmdQueue = &g_CmdQueue;

    CTRGX_BREAK_UNLESS(createTask(0x200, 0, interruptHandler, (void*)GX_INTR_PSC0) != NULL);
    CTRGX_BREAK_UNLESS(createTask(0x200, 0, interruptHandler, (void*)GX_INTR_PSC1) != NULL);
    CTRGX_BREAK_UNLESS(createTask(0x200, 0, interruptHandler, (void*)GX_INTR_PPF) != NULL);
    CTRGX_BREAK_UNLESS(createTask(0x200, 0, interruptHandler, (void*)GX_INTR_P3D) != NULL);
    return true;
}

void ctrgxs_cleanup(State* state) {
    CTRGX_ASSERT(state);

    do {
        __ldrexb(g_ShouldTerminateThreads);
    } while (__strexb(&g_ShouldTerminateThreads, 1));

    GFX_signalAllEvents();

    // This is absolutely horrible.
    while (g_RunningThreads) {
        yieldTask();
        wait_cycles(16);
    }

    state->intrQueue = NULL;
    state->cmdQueue = NULL;

    CV_Destroy(&state->platform.haltCV);
    CV_Destroy(&state->platform.completionCV);
    deleteMutex(state->platform.lock);
}

void ctrgxs_enter_critical_section(State* state, u32 op) {
    CTRGX_ASSERT(state);

    if (op & ~SAFE_OPS) {
        CTRGX_BREAK_UNLESS(lockMutex(state->platform.lock) == KRES_OK);
    }
}

void ctrgxs_exit_critical_section(State* state, u32 op) {
    CTRGX_ASSERT(state);

    if (op & ~SAFE_OPS) {
        CTRGX_BREAK_UNLESS(unlockMutex(state->platform.lock) == KRES_OK);
    }
}

void ctrgxs_set_intr_cb(State* state, GXIntr intrID, InterruptCallback cb) {
    (void)state;

    // DMA command is not supported.
    if (intrID == GX_INTR_DMA)
        return;

    u32* p = (u32*)&g_IntrCallbacks[cbIndexForIntr(intrID)];

    do {
        __ldrex(p);
    } while (__strex(p, (u32)cb));
}

void ctrgxs_clear_intr_cb(State* state, GXIntr intrID) { ctrgxs_set_intr_cb(state, intrID, NULL); }

void ctrgxs_wait_intr(State* state, GXIntr intrID) {
    (void)state;

    // DMA command is not supported.
    if (intrID >= GX_INTR_DMA)
        return;

    GFX_waitForEvent((GfxEvent)intrID);
}

void ctrgxs_clear_intr(State* state, GXIntr intrID) {
    (void)state;

    // DMA command is not supported.
    if (intrID >= GX_INTR_DMA)
        return;

    GFX_clearEvent((GfxEvent)intrID);
}

void ctrgxs_exec_commands(State* state) {
    CTRGX_ASSERT(state);

    GXCmdQueue* cmdQueue = state->cmdQueue;
    CTRGX_ASSERT(cmdQueue);

    state->platform.halted = false;

    GXCmd cmd;
    while (ctrgxCmdQueuePop(cmdQueue, &cmd)) {
        execCommand(&cmd);

        if (cmd.header & CTRGX_CMDHEADER_FLAG_LAST)
            break;
    }

    ctrgxCmdQueueHalt(cmdQueue, true);
}

void ctrgxs_wait_command_completion(State* state) {
    CTRGX_ASSERT(state);
    
    const GXCmdBuffer* cmdBuffer = state->cmdBuffer;
    while (cmdBuffer && cmdBuffer->count) {
        CV_Wait(&state->platform.completionCV, state->platform.lock);
        cmdBuffer = state->cmdBuffer;
    }
}

void ctrgxs_signal_command_completion(State* state) {
    CTRGX_ASSERT(state);
    CTRGX_ASSERT(!state->cmdBuffer || !state->cmdBuffer->count);
    
    CV_Broadcast(&state->platform.completionCV);
}

void ctrgxs_request_halt(State* state, bool wait) {
    CTRGX_ASSERT(state);

    if (!state->platform.halted) {
        state->platform.haltRequested = true;

        if (wait) {
            while (!state->platform.halted)
                CV_Wait(&state->platform.haltCV, state->platform.lock);
        }
    }
}

bool ctrgxs_signal_halt(State* state) {
    CTRGX_ASSERT(state);

    state->platform.halted = true;

    if (state->platform.haltRequested) {
        state->platform.haltRequested = false;
        CV_Broadcast(&state->platform.haltCV);
        return false;
    }

    return true;
}