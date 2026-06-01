/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CTR/Assert.h>
#include <CTR/Unreachable.h>
#include <CTR/Sync.h>

#include <KYGX/GX.h>

#include "GXServer.h"
#include "BatchQueue.h"

#define NUM_INTRS 6

#define INTR_PDC0 0
#define INTR_PDC1 1
#define INTR_PSC 2
#define INTR_PPF 3
#define INTR_P3D 4
#define INTR_DMA 5

static uint32_t g_Refc;

static CTRMtx* g_IntrMtx;
static CTRCV* g_IntrCVs[NUM_INTRS];
static bool g_IntrFlags[NUM_INTRS];

static CTRMtx* g_BatchMtx;
static BatchQueue g_BatchQueue;

static CTRCV* g_CompletionCV;
static CTRCV* g_HaltCV;
static bool g_Halt = false;
static bool g_Executing = false;

static size_t indexForIntr(KYGXIntr intrID) {
    switch (intrID) {
        case KYGXIntr_PDC0:
            return INTR_PDC0;
        case KYGXIntr_PDC1:
            return INTR_PDC1;
        case KYGXIntr_PSC:
            return INTR_PSC;
        case KYGXIntr_PPF:
            return INTR_PPF;
        case KYGXIntr_P3D:
            return INTR_P3D;
        case KYGXIntr_DMA:
            return INTR_DMA;
        default:
            CTR_UNREACHABLE("Unknown interrupt %u", (size_t)intrID);
    }
}

static KYGXIntr intrForIndex(size_t idx) {
    switch (idx) {
         case INTR_PDC0:
            return KYGXIntr_PDC0;
        case INTR_PDC1:
            return KYGXIntr_PDC1;
        case INTR_PSC:
            return KYGXIntr_PSC;
        case INTR_PPF:
            return KYGXIntr_PPF;
        case INTR_P3D:
            return KYGXIntr_P3D;
        case INTR_DMA:
            return KYGXIntr_DMA;
        default:
            CTR_UNREACHABLE("Unknown interrupt %u", idx);
    }
}

// Try executing the next batch. Returns NO_CMDS if no commands, BUSY if busy. Note: thread-unsafe.
static GXExecState tryExecNextBatch(void) {
    CmdIterator it;

    const BatchQueueError qErr = CmdIteratorInit(&it, &g_BatchQueue);

    // Of course, only execute if we have something.
    if (qErr == BatchQueueError_Success) {
        const GXExecState state = GXServerExec(&it);

        if (state == GXExecState_Success) {
            g_Executing = true;
        } else {
            CTR_BREAK_IF(state != GXExecState_Busy);
        }

        return state;
    }

    CTR_ASSERT(qErr == BatchQueueError_NoCommands);
    return GXExecState_NoCommands;
}

static void onInterrupt(KYGXIntr intrID) {
    const size_t index = indexForIntr(intrID);

    ctrMtxAcquire(g_IntrMtx);

    if (!g_IntrFlags[index]) {
        g_IntrFlags[index] = true;
        ctrCVBroadcast(g_IntrCVs[index]);
    }

    ctrMtxRelease(g_IntrMtx);
}

static void onBatchCompleted(void) {
    ctrMtxAcquire(g_BatchMtx);
    
    g_Executing = false;

    // Advance to the next batch.
    KYGXBatchCallback cb;
    void* cbData;

    CTR_BREAK_IF(BatchQueuePop(&g_BatchQueue, &cb, &cbData) != BatchQueueError_Success);

    // If we were asked to halt, do so.
    if (g_Halt) {
        ctrCVBroadcast(g_HaltCV);
    } else {
        // Otherwise, execute the next batch.
        GXExecState state = tryExecNextBatch();
        
        if (state == GXExecState_NoCommands) {
            // We have executed all batches.
            ctrCVBroadcast(g_CompletionCV);
        } else {
            while (state == GXExecState_Busy) {
                // We know batch processing has ended, let's give the server time to update its state.
                ctrYield();
                state = tryExecNextBatch();
            }

            CTR_BREAK_IF(state != GXExecState_Success);
        }
    }

    ctrMtxRelease(g_BatchMtx);

    // Execute the callback.
    if (cb)
        cb(cbData);
}

KYGXError kygxInit(size_t maxCommands) {
    if (CTR_ATOMIC_POST_INC(g_Refc))
        return KYGXError_Success;

    // Init batch queue.
    const BatchQueueError qErr = BatchQueueInit(&g_BatchQueue, maxCommands);
    if (qErr != BatchQueueError_Success) {
        CTR_ASSERT(qErr == BatchQueueError_NoMemory);
        return KYGXError_NoMemory;
    }

    // Init GX server.
    GXServerInit();

    // Allocate resources.
    g_IntrMtx = ctrMtxCreate();

    for (size_t i = 0; i < NUM_INTRS; ++i)
        g_IntrCVs[i] = ctrCVCreate();

    g_BatchMtx = ctrMtxCreate();
    g_CompletionCV = ctrCVCreate();
    g_HaltCV = ctrCVCreate();

    // Set initial state.
    for (size_t i = 0; i < NUM_INTRS; ++i)
        g_IntrFlags[i] = false;

    g_Halt = false;
    g_Executing = false;

    GXServerSetCallbacks(onInterrupt, onBatchCompleted);
    return KYGXError_Success;
}

void kygxExit(void) {
    if (CTR_ATOMIC_PRE_DEC(g_Refc))
        return;

    kygxSetHalt(true, true);

    GXServerSetCallbacks(NULL, NULL);

    ctrCVDestroy(g_HaltCV);
    ctrCVDestroy(g_CompletionCV);
    ctrMtxDestroy(g_BatchMtx);
    ctrMtxDestroy(g_IntrMtx);

    GXServerExit();
    BatchQueueDestroy(&g_BatchQueue);
}

bool kygxIsInitialized(void) { return g_Refc > 0; }

void kygxClearIntr(KYGXIntr intrID) {
    const size_t index = indexForIntr(intrID);
    ctrMtxAcquire(g_IntrMtx);
    g_IntrFlags[index] = false;
    ctrMtxRelease(g_IntrMtx);
}

void kygxWaitIntr(KYGXIntr intrID) {
    const size_t index = indexForIntr(intrID);

    ctrMtxAcquire(g_IntrMtx);

    while (!g_IntrFlags[index])
        ctrCVWait(g_IntrCVs[index], g_IntrMtx);

    ctrMtxRelease(g_IntrMtx);
}

KYGXError kygxPushBatch(const KYGXCmd* commands, size_t numCommands, KYGXBatchCallback cb, void* cbData) {
    ctrMtxAcquire(g_BatchMtx);

    const bool shouldExec = !g_Halt && BatchQueueIsEmpty(&g_BatchQueue);
    const BatchQueueError qErr = BatchQueuePush(&g_BatchQueue, commands, numCommands, cb, cbData);

    // Kickstart execution if needed.
    if (qErr == KYGXError_Success && shouldExec) {
        // Server should not be busy at this point.
        CTR_BREAK_IF(tryExecNextBatch() != GXExecState_Success);
    }

    ctrMtxRelease(g_BatchMtx);

    switch (qErr) {
        case BatchQueueError_Success:
            return KYGXError_Success;
        case BatchQueueError_NoMemory:
            return KYGXError_NoMemory;
        case BatchQueueError_NoCommands:
            return KYGXError_NoCommands;
        default:
            CTR_UNREACHABLE("Unknown error code %u", (uint32_t)qErr);
    }
}

void kygxWaitCompletion(void) {
    ctrMtxAcquire(g_BatchMtx);

    while (!BatchQueueIsEmpty(&g_BatchQueue))
        ctrCVWait(g_CompletionCV, g_BatchMtx);

    ctrMtxRelease(g_BatchMtx);
}

// Note: thread-unsafe.
static void doHalt(bool wait) {
    g_Halt = true;

    if (wait) {
        while (g_Executing)
            ctrCVWait(g_HaltCV, g_BatchMtx);
    }
}

// Note: thread-unsafe.
static void undoHalt(void) {
    g_Halt = false;
    const GXExecState state = tryExecNextBatch();

    if (state == GXExecState_NoCommands)
        return;

    // Server should not be busy at this point.
    CTR_BREAK_IF(state != GXExecState_Success);
}

void kygxSetHalt(bool halt, bool wait) {
    ctrMtxAcquire(g_BatchMtx);

    if (halt) {
        doHalt(wait);
    } else {
        undoHalt();
    }

    ctrMtxRelease(g_BatchMtx);
}

static size_t intrIdxForSyncCmd(uint32_t header) {
    switch (header & 0xFF) {
        case KYGX_CMD_REQUESTDMA:
            return INTR_DMA;
        case KYGX_CMD_PROCESSCOMMANDLIST:
            return INTR_P3D;
        case KYGX_CMD_MEMORYFILL:
            return INTR_PSC;
        case KYGX_CMD_DISPLAYTRANSFER:
        case KYGX_CMD_TEXTURECOPY:
            return INTR_PPF;
        case KYGX_CMD_FLUSHCACHEREGIONS:
            // FlushCacheRegions doesn't trigger any interrupt.
            return -1;
        default:
            CTR_UNREACHABLE("Unknown command %u", header & 0xFF);
    }

    return -1;
}

void kygxExecSync(const KYGXCmd* command) {
    CTR_ASSERT(command);

    const size_t intrIdx = intrIdxForSyncCmd(command->header);

    ctrMtxAcquire(g_BatchMtx);

    // Halt execution of batches.
    doHalt(true);
    GXServerSetCallbacks(onInterrupt, NULL);

    // Clear interrupt state if we need to wait for it.
    if (intrIdx != -1)
        kygxClearIntr(intrForIndex(intrIdx));

    // Construct iterator.
    CmdIterator it;
    it.cmdList = command;
    it.index = 0;
    it.count = 1;
    it.queueCapacity = 1;

    // Execute command.
    GXExecState state = GXServerExec(&it);
    
    while (state == GXExecState_Busy) {
        ctrYield();
        state = GXServerExec(&it);
    }

    CTR_BREAK_IF(state != GXExecState_Success);

    // Wait for termination.
    if (intrIdx != -1)
        kygxWaitIntr(intrForIndex(intrIdx));

    // Resume processing.
    GXServerSetCallbacks(onInterrupt, onBatchCompleted);
    undoHalt();

    ctrMtxRelease(g_BatchMtx);
}