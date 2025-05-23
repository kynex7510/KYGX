#include "GX/GX.h"
#include "State.h"

static State g_GlobalState = {};
static u32 g_Refc = 0;

static GXIntr intrForCmd(const GXCmd* cmd) {
    switch (cmd->header & 0xFF) {
        case CTRGX_CMDID_REQUESTDMA:
            return GX_INTR_DMA;
        case CTRGX_CMDID_PROCESSCOMMANDLIST:
            return GX_INTR_P3D;
        case CTRGX_CMDID_DISPLAYTRANSFER:
        case CTRGX_CMDID_TEXTURECOPY:
            return GX_INTR_PPF;
    }

    // When using two buffers MemoryFill only triggers PSC0.
    if ((cmd->header & 0xFF) == CTRGX_CMDID_MEMORYFILL) {
        const bool buf0 = cmd->params[0];
        const bool buf1 = cmd->params[3];
        return buf1 && !buf0 ? GX_INTR_PSC1 : GX_INTR_PSC0;
    }

    return GX_INTR_UNKNOWN;
}

static void triggerCommandHandling(void) {
    GXCmdQueue* cmdQueue = g_GlobalState.cmdQueue;
    CTRGX_ASSERT(cmdQueue);

    ctrgxCmdQueueClearError(cmdQueue);
    ctrgxCmdQueueHalt(cmdQueue, false);
    ctrgxs_exec_commands(&g_GlobalState);
}

static bool enqueueCommands(void) {
    GXCmdQueue* cmdQueue = g_GlobalState.cmdQueue;
    CTRGX_ASSERT(cmdQueue);

    if (!ctrgxCmdQueueIsHalted(cmdQueue))
        return true;

    GXCmdBuffer* cmdBuffer = g_GlobalState.cmdBuffer;
    if (!cmdBuffer || !cmdBuffer->count) {
        ctrgxs_signal_command_completion(&g_GlobalState);
        return true;
    }

    // The buffer must be finalized.
    if (!ctrgxCmdBufferIsFinalized(cmdBuffer))
        return false;

    // Write commands to queue.
    u8 numCommands = 0;
    for (u8 i = 0; i < cmdBuffer->count; ++i) {
        GXCmd* cmd;
        CTRGX_BREAK_UNLESS(ctrgxCmdBufferPeek(cmdBuffer, i, &cmd, NULL, NULL));

        if (intrForCmd(cmd) != GX_INTR_UNKNOWN)
            ++numCommands;

        CTRGX_BREAK_UNLESS(ctrgxCmdQueueAdd(cmdQueue, cmd));

        if (cmd->header & CTRGX_CMDHEADER_FLAG_LAST)
            break;
    }

    // Update state.
    g_GlobalState.pendingCommands = numCommands;
    g_GlobalState.completedCommands = 0;

    triggerCommandHandling();

    // TODO: Handle the case where no interrupts are signaled (eg. single FlushCacheRegions).
    CTRGX_ASSERT(numCommands);
    //

    return true;
}

static void onInterrupt(GXIntr intrID) {
    // We don't care about PDC.
    if (intrID == GX_INTR_PDC0 || intrID == GX_INTR_PDC1)
        return;

    const u32 criticalOp = STATEOP_FIELD_ACCESS | STATEOP_EXEC_COMMANDS | STATEOP_COMMAND_COMPLETION | STATEOP_HALT;
    ctrgxs_enter_critical_section(&g_GlobalState, criticalOp);

    // Update state.
    ++g_GlobalState.completedCommands;
    --g_GlobalState.pendingCommands;

    GXCmdBuffer* cmdBuffer = g_GlobalState.cmdBuffer;
    CTRGX_ASSERT(cmdBuffer);

    GXCallback cb = NULL;
    void* data = NULL;

    CTRGX_BREAK_UNLESS(ctrgxCmdBufferPeek(cmdBuffer, 0, NULL, &cb, &data));
    ctrgxCmdBufferAdvance(cmdBuffer, 1);

    // Handle batch termination.
    if (!g_GlobalState.pendingCommands) {
        // It's possible that, at this point, GSP still hasn't halted. Let's do it ourselves.
        GXCmdQueue* cmdQueue = g_GlobalState.cmdQueue;
        CTRGX_ASSERT(cmdQueue && !cmdQueue->count);
        ctrgxCmdQueueHalt(cmdQueue, true);

        // Invoke callback.
        if (cb)
            cb(data);

        // Proceed with the next batch if we weren't asked to halt.
        if (ctrgxs_signal_halt(&g_GlobalState)) {
            CTRGX_BREAK_UNLESS(enqueueCommands());
        }
    }

    ctrgxs_exit_critical_section(&g_GlobalState, criticalOp);
}

static void resetCmdQueue(bool halt) {
    GXCmdQueue* cmdQueue = g_GlobalState.cmdQueue;
    CTRGX_ASSERT(cmdQueue);

    ctrgxCmdQueueClear(cmdQueue);
    ctrgxCmdQueueClearError(cmdQueue);
    ctrgxCmdQueueHalt(cmdQueue, halt);
}

bool ctrgxInit(void) {
    if (__atomic_fetch_add(&g_Refc, 1, __ATOMIC_SEQ_CST))
        return true;

    if (ctrgxs_init(&g_GlobalState)) {
        resetCmdQueue(true);
        return true;
    }

    __atomic_sub_fetch(&g_Refc, 1, __ATOMIC_SEQ_CST);
    return false;
}

void ctrgxExit(void) {
    if (!__atomic_sub_fetch(&g_Refc, 1, __ATOMIC_SEQ_CST)) {
        resetCmdQueue(false);
        ctrgxs_cleanup(&g_GlobalState);
    }
}

GXCmdBuffer* ctrgxExchangeCmdBuffer(GXCmdBuffer* b, bool flush) {
    const u32 criticalOp = STATEOP_FIELD_ACCESS | (flush ? STATEOP_COMMAND_COMPLETION : STATEOP_HALT);
    ctrgxs_enter_critical_section(&g_GlobalState, criticalOp);

    if (flush) {
        ctrgxs_wait_command_completion(&g_GlobalState);
    } else {
        ctrgxs_request_halt(&g_GlobalState, true);
    }
    
    GXCmdBuffer* old = g_GlobalState.cmdBuffer;
    g_GlobalState.cmdBuffer = b;

    ctrgxs_exit_critical_section(&g_GlobalState, criticalOp);
    return old;
}

void ctrgxLock(void) { ctrgxs_enter_critical_section(&g_GlobalState, STATEOP_FIELD_ACCESS | STATEOP_EXEC_COMMANDS); }

void ctrgxUnlock(bool exec) {
    if (exec) {
        CTRGX_BREAK_UNLESS(enqueueCommands());
    }

    ctrgxs_exit_critical_section(&g_GlobalState, STATEOP_FIELD_ACCESS | STATEOP_EXEC_COMMANDS);
}

GXIntrQueue* ctrgxGetIntrQueue(void) { return g_GlobalState.intrQueue; }
GXCmdQueue* ctrgxGetCmdQueue(void) { return g_GlobalState.cmdQueue; }
GXCmdBuffer* ctrgxGetCmdBuffer(void) { return g_GlobalState.cmdBuffer; }

void ctrgxWaitIntr(GXIntr intrID) {
    ctrgxs_enter_critical_section(&g_GlobalState, STATEOP_INTR);
    ctrgxs_wait_intr(&g_GlobalState, intrID);
    ctrgxs_exit_critical_section(&g_GlobalState, STATEOP_INTR);
}

void ctrgxClearIntr(GXIntr intrID) {
    ctrgxs_enter_critical_section(&g_GlobalState, STATEOP_INTR);
    ctrgxs_clear_intr(&g_GlobalState, intrID);
    ctrgxs_exit_critical_section(&g_GlobalState, STATEOP_INTR);
}

bool ctrgxFlushBufferedCommands(void) {
    ctrgxs_enter_critical_section(&g_GlobalState, STATEOP_FIELD_ACCESS | STATEOP_EXEC_COMMANDS);
    const bool b = enqueueCommands();
    ctrgxs_exit_critical_section(&g_GlobalState, STATEOP_FIELD_ACCESS | STATEOP_EXEC_COMMANDS);
    return b;
}

void ctrgxWaitCompletion(void) {
    ctrgxs_enter_critical_section(&g_GlobalState, STATEOP_COMMAND_COMPLETION);
    ctrgxs_wait_command_completion(&g_GlobalState);
    ctrgxs_exit_critical_section(&g_GlobalState, STATEOP_COMMAND_COMPLETION);
}

void ctrgxHalt(bool wait) {
    ctrgxs_enter_critical_section(&g_GlobalState, STATEOP_HALT);
    ctrgxs_request_halt(&g_GlobalState, wait);
    ctrgxs_exit_critical_section(&g_GlobalState, STATEOP_HALT);
}

CTRGX_EXTERN void ctrgxExecSync(const GXCmd* cmd) {
    GXCmd cmdCopy;
    memcpy(&cmdCopy, cmd, sizeof(GXCmd));
    cmdCopy.header |= CTRGX_CMDHEADER_FLAG_LAST;

    const GXIntr intrID = intrForCmd(&cmdCopy);
    const u32 criticalOp = STATEOP_FIELD_ACCESS | STATEOP_HALT | STATEOP_INTR | STATEOP_EXEC_COMMANDS;
    ctrgxs_enter_critical_section(&g_GlobalState, criticalOp);

    // Wait batch processing.
    ctrgxs_request_halt(&g_GlobalState, true);

    if (intrID != GX_INTR_UNKNOWN) {
        ctrgxs_disable_intr_cb(&g_GlobalState, intrID);
        ctrgxs_clear_intr(&g_GlobalState, intrID);
    }

    // Execute command.
    CTRGX_BREAK_UNLESS(ctrgxCmdQueueAdd(g_GlobalState.cmdQueue, &cmdCopy));
    triggerCommandHandling();

    // Wait for termination.
    if (intrID != GX_INTR_UNKNOWN) {
        ctrgxs_wait_intr(&g_GlobalState, intrID);
        ctrgxs_enable_intr_cb(&g_GlobalState, intrID);
    }

    // Resume command buffer processing.
    if (ctrgxs_signal_halt(&g_GlobalState)) {
        CTRGX_BREAK_UNLESS(enqueueCommands());
    }
    
    ctrgxs_exit_critical_section(&g_GlobalState, criticalOp);
}