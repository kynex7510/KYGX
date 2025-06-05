#include <GX/GX.h>
#include "State.h"

static State g_GlobalState = {};
static u32 g_Refc = 0;

static GXIntr intrForCmd(const GXCmd* cmd) {
    switch (cmd->header & 0xFF) {
        case KYGX_CMDID_REQUESTDMA:
            return GX_INTR_DMA;
        case KYGX_CMDID_PROCESSCOMMANDLIST:
            return GX_INTR_P3D;
        case KYGX_CMDID_DISPLAYTRANSFER:
        case KYGX_CMDID_TEXTURECOPY:
            return GX_INTR_PPF;
    }

    // When using two buffers MemoryFill only triggers PSC0.
    if ((cmd->header & 0xFF) == KYGX_CMDID_MEMORYFILL) {
        const bool buf0 = cmd->params[0];
        const bool buf1 = cmd->params[3];
        return buf1 && !buf0 ? GX_INTR_PSC1 : GX_INTR_PSC0;
    }

    return GX_INTR_UNKNOWN;
}

static void triggerCommandHandling(void) {
    GXCmdQueue* cmdQueue = g_GlobalState.cmdQueue;
    KYGX_ASSERT(cmdQueue);

    kygxCmdQueueClearError(cmdQueue);
    kygxCmdQueueClearHalt(cmdQueue);
    kygxs_exec_commands(&g_GlobalState);
}

static bool enqueueCommands(void) {
    // Do not enqueue commands if there's any pending.
    if (g_GlobalState.pendingCommands)
        return true;

    GXCmdBuffer* cmdBuffer = g_GlobalState.cmdBuffer;
    if (!cmdBuffer || !cmdBuffer->count) {
        kygxs_signal_command_completion(&g_GlobalState);
        return true;
    }

    // The buffer must be finalized.
    if (!kygxCmdBufferIsFinalized(cmdBuffer))
        return false;

    // Write commands to queue.
    GXCmdQueue* cmdQueue = g_GlobalState.cmdQueue;
    KYGX_ASSERT(cmdQueue && !cmdQueue->count && cmdQueue->status == KYGX_CMDQUEUE_STATUS_HALTED);

    for (u8 i = 0; i < cmdBuffer->count; ++i) {
        GXCmd* cmd;
        GXCallback cb;
        void* cbData;
        KYGX_BREAK_UNLESS(kygxCmdBufferPeek(cmdBuffer, i, &cmd, &cb, &cbData));
        KYGX_BREAK_UNLESS(intrForCmd(cmd) != GX_INTR_UNKNOWN);
        KYGX_BREAK_UNLESS(kygxCmdQueueAdd(cmdQueue, cmd));

        if (cmd->header & KYGX_CMDHEADER_FLAG_LAST) {
            g_GlobalState.currentCb = cb;
            g_GlobalState.currentCbData = cbData;
            break;
        }
    }

    // Update state.
    kygxCmdBufferAdvance(cmdBuffer, cmdQueue->count);
    g_GlobalState.pendingCommands = cmdQueue->count;
    g_GlobalState.completedCommands = 0;

    triggerCommandHandling();
    return true;
}

static void onInterrupt(GXIntr intrID) {
    // We don't care about PDC.
    if (intrID == GX_INTR_PDC0 || intrID == GX_INTR_PDC1)
        return;

    const u32 criticalOp = STATEOP_FIELD_ACCESS | STATEOP_EXEC_COMMANDS | STATEOP_COMMAND_COMPLETION | STATEOP_HALT;
    kygxs_enter_critical_section(&g_GlobalState, criticalOp);

    // Update state.
    ++g_GlobalState.completedCommands;
    --g_GlobalState.pendingCommands;

    // Handle batch termination.
    if (!g_GlobalState.pendingCommands) {
        // It's possible that, at this point, GSP still hasn't halted.
        GXCmdQueue* cmdQueue = g_GlobalState.cmdQueue;
        KYGX_ASSERT(cmdQueue && !cmdQueue->count);
        kygxCmdQueueWaitHalt(cmdQueue);

        // Invoke callback.
        if (g_GlobalState.currentCb) {
            g_GlobalState.currentCb(g_GlobalState.currentCbData);
            g_GlobalState.currentCb = NULL;
            g_GlobalState.currentCbData = NULL;
        }

        // Proceed with the next batch if we weren't asked to halt.
        if (kygxs_signal_halt(&g_GlobalState))
            enqueueCommands();
    }

    kygxs_exit_critical_section(&g_GlobalState, criticalOp);
}

static void resetCmdQueue(bool halt) {
    GXCmdQueue* cmdQueue = g_GlobalState.cmdQueue;
    KYGX_ASSERT(cmdQueue);

    kygxCmdQueueClearCommands(cmdQueue);
    kygxCmdQueueClearError(cmdQueue);

    if (halt) {
        kygxCmdQueueSetHalt(cmdQueue);
    } else {
        kygxCmdQueueClearHalt(cmdQueue);
    }
}

bool kygxInit(void) {
    if (__atomic_fetch_add(&g_Refc, 1, __ATOMIC_SEQ_CST))
        return true;

    if (kygxs_init(&g_GlobalState)) {
        resetCmdQueue(true);
        g_GlobalState.currentCb = NULL;
        g_GlobalState.currentCbData = NULL;
        g_GlobalState.pendingCommands = 0;
        g_GlobalState.completedCommands = 0;
        return true;
    }

    __atomic_sub_fetch(&g_Refc, 1, __ATOMIC_SEQ_CST);
    return false;
}

void kygxExit(void) {
    kygxHalt(true);

    if (!__atomic_sub_fetch(&g_Refc, 1, __ATOMIC_SEQ_CST)) {
        resetCmdQueue(false);
        kygxs_cleanup(&g_GlobalState);
    }
}

GXCmdBuffer* kygxExchangeCmdBuffer(GXCmdBuffer* b, bool flush) {
    const u32 criticalOp = STATEOP_FIELD_ACCESS | (flush ? STATEOP_COMMAND_COMPLETION : STATEOP_HALT);
    kygxs_enter_critical_section(&g_GlobalState, criticalOp);

    if (flush) {
        kygxs_wait_command_completion(&g_GlobalState);
    } else {
        kygxs_request_halt(&g_GlobalState, true);
    }
    
    GXCmdBuffer* old = g_GlobalState.cmdBuffer;
    g_GlobalState.cmdBuffer = b;

    kygxs_exit_critical_section(&g_GlobalState, criticalOp);
    return old;
}

void kygxLock(void) { kygxs_enter_critical_section(&g_GlobalState, STATEOP_FIELD_ACCESS | STATEOP_EXEC_COMMANDS); }

bool kygxUnlock(bool exec) {
    const bool b = exec ? enqueueCommands() : true;
    kygxs_exit_critical_section(&g_GlobalState, STATEOP_FIELD_ACCESS | STATEOP_EXEC_COMMANDS);
    return b;
}

GXIntrQueue* kygxGetIntrQueue(void) { return g_GlobalState.intrQueue; }
GXCmdQueue* kygxGetCmdQueue(void) { return g_GlobalState.cmdQueue; }
GXCmdBuffer* kygxGetCmdBuffer(void) { return g_GlobalState.cmdBuffer; }

void kygxWaitIntr(GXIntr intrID) {
    kygxs_enter_critical_section(&g_GlobalState, STATEOP_INTR);
    kygxs_wait_intr(&g_GlobalState, intrID);
    kygxs_exit_critical_section(&g_GlobalState, STATEOP_INTR);
}

void kygxClearIntr(GXIntr intrID) {
    kygxs_enter_critical_section(&g_GlobalState, STATEOP_INTR);
    kygxs_clear_intr(&g_GlobalState, intrID);
    kygxs_exit_critical_section(&g_GlobalState, STATEOP_INTR);
}

bool kygxFlushBufferedCommands(void) {
    kygxs_enter_critical_section(&g_GlobalState, STATEOP_FIELD_ACCESS | STATEOP_EXEC_COMMANDS);
    const bool b = enqueueCommands();
    kygxs_exit_critical_section(&g_GlobalState, STATEOP_FIELD_ACCESS | STATEOP_EXEC_COMMANDS);
    return b;
}

void kygxWaitCompletion(void) {
    kygxs_enter_critical_section(&g_GlobalState, STATEOP_COMMAND_COMPLETION);
    kygxs_wait_command_completion(&g_GlobalState);
    kygxs_exit_critical_section(&g_GlobalState, STATEOP_COMMAND_COMPLETION);
}

void kygxHalt(bool wait) {
    kygxs_enter_critical_section(&g_GlobalState, STATEOP_HALT);
    kygxs_request_halt(&g_GlobalState, wait);
    kygxs_exit_critical_section(&g_GlobalState, STATEOP_HALT);
}

void kygxExecSync(const GXCmd* cmd) {
    GXCmd cmdCopy;
    memcpy(&cmdCopy, cmd, sizeof(GXCmd));
    cmdCopy.header |= KYGX_CMDHEADER_FLAG_LAST;

    const GXIntr intrID = intrForCmd(&cmdCopy);
    const u32 criticalOp = STATEOP_FIELD_ACCESS | STATEOP_HALT | STATEOP_INTR | STATEOP_EXEC_COMMANDS;
    kygxs_enter_critical_section(&g_GlobalState, criticalOp);

    // Wait batch processing.
    kygxs_request_halt(&g_GlobalState, true);

    if (intrID != GX_INTR_UNKNOWN) {
        kygxs_disable_intr_cb(&g_GlobalState, intrID);
        kygxs_clear_intr(&g_GlobalState, intrID);
    }

    // Execute command.
    GXCmdQueue* cmdQueue = g_GlobalState.cmdQueue;
    KYGX_ASSERT(cmdQueue);

    KYGX_BREAK_UNLESS(kygxCmdQueueAdd(cmdQueue, &cmdCopy));
    triggerCommandHandling();

    // Wait for termination.
    if (intrID != GX_INTR_UNKNOWN) {
        kygxs_wait_intr(&g_GlobalState, intrID);
        kygxs_enable_intr_cb(&g_GlobalState, intrID);
    } else {
        kygxCmdQueueWaitHalt(cmdQueue);
    }

    // Resume command buffer processing.
    if (kygxs_signal_halt(&g_GlobalState)) {
        KYGX_BREAK_UNLESS(enqueueCommands());
    }
    
    kygxs_exit_critical_section(&g_GlobalState, criticalOp);
}