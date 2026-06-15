/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CTR/Unreachable.h>
#include <CTR/Assert.h>
#include <CTR/Sync.h>

#include "BatchRunner.h"
#include "CmdQueue.h"
#include "Interrupt.h"

#include <string.h>

static RunnerOnInterrupt g_UserOnInterrupt = NULL;
static RunnerOnBatchCompleted g_UserOnBatchCompleted = NULL;
static uint8_t g_NumPendingCommands = 0;

void IntrCallback(KYGXIntr intrID) {
    // Flush is a pseudo interrupt and shall not be signaled.
    if (CTR_LIKELY(g_UserOnInterrupt && intrID != KYGXIntr_Flush))
        g_UserOnInterrupt(intrID);

    // Nothing else to do with PDC.
    if (CTR_LIKELY(intrID == KYGXIntr_PDC0 || intrID == KYGXIntr_PDC1))
        return;

    // We should not be getting spurious interrupts.
    CTR_ASSERT(g_NumPendingCommands > 0);

    // Update state.
    --g_NumPendingCommands;

    // Handle batch termination.
    if (--g_NumPendingCommands) {
        // It's possible that, at this point, the queue is still busy.
        // This can happen if the last command is a flush command, for example.
        // This also handles a batch made of flush commands only.
        while (CmdQueueIsBusy())
            ctrYield();

        // Invoke callback.
        if (CTR_LIKELY(g_UserOnBatchCompleted))
            g_UserOnBatchCompleted();
    }
}

void BatchRunnerInit(void) {
    g_UserOnInterrupt = NULL;
    g_UserOnBatchCompleted = NULL;
    g_NumPendingCommands = 0;

    IntrInit();
    CmdQueueInit();
}

void BatchRunnerExit(void) {
    CmdQueueExit();
    IntrExit();
}

void BatchRunnerSetCallbacks(RunnerOnInterrupt onInterrupt, RunnerOnBatchCompleted onBatchCompleted) {
    g_UserOnInterrupt = onInterrupt;
    g_UserOnBatchCompleted = onBatchCompleted;
}

ExecState BatchRunnerExec(CmdIterator* it) {
    CTR_ASSERT(it);

    // Check if we can add commands.
    size_t numCommands = CmdIteratorCount(it);

    if (!numCommands)
        return ExecState_NoCommands;

    if (numCommands > CMDQUEUE_MAX_CMDS)
        return ExecState_NoMemory;

    if (CmdQueueIsBusy())
        return ExecState_Busy;

    // Add commands.
    size_t numFlushCommands = 0;

    for (size_t i = 0; i < numCommands; ++i) {
        const KYGXCmd* src = CmdIteratorNext(it);
        CTR_ASSERT(src);

        CmdQueueAdd(src);

        if ((src->header & 0xFF) == KYGX_CMD_FLUSHCACHEREGIONS)
            ++numFlushCommands;
    }

    g_NumPendingCommands = numCommands - numFlushCommands;

    // Execute commands.
    CmdQueueTriggerHandling();

    // If we have no interrupts, simulate it.
    if (!g_NumPendingCommands) {
        g_NumPendingCommands = 1;
        IntrSignalFlush();
    }

    return ExecState_Success;
}