/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CTR/Unreachable.h>
#include <CTR/Sync.h>

#include <KYGX/GX.h>

#include "GXServer.h"
#include "BatchQueue.h"

#define NUM_INTRS 7

#define INTR_PSC0 0
#define INTR_PSC1 1
#define INTR_PDC0 2
#define INTR_PDC1 3
#define INTR_PPF 4
#define INTR_P3D 5
#define INTR_DMA 6

static uint32_t g_Refc;

static CTRMtx* g_IntrMtx;
static CTRCV* g_IntrCVs[NUM_INTRS];
static bool g_IntrFlags[NUM_INTRS];

static CTRMtx* g_BatchMtx;
static BatchQueue g_BatchQueue;

static CTRCV* g_CompletionCV;
static CTRCV* g_HaltCV;
static bool g_HaltRequested = false;
static bool g_Running = false;

static size_t indexForIntr(KYGXIntr intrID) {
    switch (intrID) {
        case KYGX_INTR_PSC0:
            return INTR_PSC0;
        case KYGX_INTR_PSC1:
            return INTR_PSC1;
        case KYGX_INTR_PDC0:
            return INTR_PDC0;
        case KYGX_INTR_PDC1:
            return INTR_PDC1;
        case KYGX_INTR_PPF:
            return INTR_PPF;
        case KYGX_INTR_P3D:
            return INTR_P3D;
        case KYGX_INTR_DMA:
            return INTR_DMA;
        default:
            CTR_UNREACHABLE("Unknown interrupt %u", (size_t)intrID);
    }
}

static KYGXIntr intrForIndex(size_t idx) {
    switch (idx) {
        case INTR_PSC0:
            return KYGX_INTR_PSC0;
        case INTR_PSC1:
            return KYGX_INTR_PSC1;
        case INTR_PDC0:
            return KYGX_INTR_PDC0;
        case INTR_PDC1:
            return KYGX_INTR_PDC1;
        case INTR_PPF:
            return KYGX_INTR_PPF;
        case INTR_P3D:
            return KYGX_INTR_P3D;
        case INTR_DMA:
            return KYGX_INTR_DMA;
        default:
            CTR_UNREACHABLE("Unknown interrupt %u", idx);
    }
}

// Try executing the next batch. Note: thread-unsafe.
static KYGXError tryExecNextBatch(void) {
    CmdIterator it;

    KYGXError ret = CmdIteratorInit(&it, &g_BatchQueue);
    CTR_BREAK_IF(ret != KYGX_ERROR_SUCCESS && ret != KYGX_ERROR_EMPTY);

    // Of course, only execute if we have something.
    if (ret == KYGX_ERROR_SUCCESS) {
        ret = GXServerExec(&it);
        CTR_BREAK_IF(ret != KYGX_ERROR_SUCCESS && ret != KYGX_ERROR_BUSY);
    }

    return ret;
}

static void onInterrupt(KYGXIntr intrID) {
    const size_t index = indexForIntr(intrID);
    ctrMtxAcquire(g_IntrMtx);
    g_IntrFlags[index] = true;
    ctrMtxRelease(g_IntrMtx);
}

static void onBatchCompleted(void) {
    ctrMtxAcquire(g_BatchMtx);
    
    // Advance to the next batch.
    KYGXBatchCallback cb;
    void* cbData;

    KYGXError ret = BatchQueuePop(&g_BatchQueue, &cb, &cbData);
    CTR_BREAK_IF(ret != KYGX_ERROR_SUCCESS);

    // If we were asked to halt, do so.
    if (g_HaltRequested) {
        g_HaltRequested = false;
        g_Running = false;
        ctrCVBroadcast(g_HaltCV);
    } else {
        // Otherwise, execute the next batch.
        KYGXError ret = tryExecNextBatch();
        
        if (ret == KYGX_ERROR_EMPTY) {
            // We have executed all batches.
            ctrCVBroadcast(g_CompletionCV);
        } else {
            while (ret == KYGX_ERROR_BUSY) {
                // We know batch processing has ended, let's give the server time to update its state.
                ctrYield();
                ret = tryExecNextBatch();
            }

            CTR_BREAK_IF(ret != KYGX_ERROR_SUCCESS);
        }
    }

    ctrMtxRelease(g_BatchMtx);

    // Execute the callback.
    if (cb)
        cb(cbData);
}

KYGXError kygxInit(size_t maxCommands) {
    if (CTR_ATOMIC_POST_INC(g_Refc))
        return KYGX_ERROR_SUCCESS;

    // Init batch queue.
    KYGXError ret = BatchQueueInit(&g_BatchQueue, maxCommands);
    if (ret != KYGX_ERROR_SUCCESS)
        return ret;

    // Init GX server.
    ret = GXServerInit();
    if (ret != KYGX_ERROR_SUCCESS) {
        BatchQueueDestroy(&g_BatchQueue);
        return ret;
    }

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

    g_Running = true;
    g_HaltRequested = false;

    GXServerSetCallbacks(onInterrupt, onBatchCompleted);
    return KYGX_ERROR_SUCCESS;
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

    const bool shouldExec = g_Running && BatchQueueIsEmpty(&g_BatchQueue);
    const KYGXError ret = BatchQueuePush(&g_BatchQueue, commands, numCommands, cb, cbData);

    // Kickstart execution if needed.
    if (ret == KYGX_ERROR_SUCCESS && shouldExec) {
        // Server should not be busy at this point.
        CTR_BREAK_IF(tryExecNextBatch() == KYGX_ERROR_SUCCESS);
    }

    ctrMtxRelease(g_BatchMtx);
    return ret;
}

void kygxWaitCompletion(void) {
    ctrMtxAcquire(g_BatchMtx);

    while (!BatchQueueIsEmpty(&g_BatchQueue))
        ctrCVWait(g_CompletionCV, g_BatchMtx);

    ctrMtxRelease(g_BatchMtx);
}

// Note: thread-unsafe.
static void doHalt(bool wait) {
    if (g_Running) {
        g_HaltRequested = true;
        
        if (wait) {
            while (g_Running)
                ctrCVWait(g_HaltCV, g_BatchMtx);
        }
    }
}

// Note: thread-unsafe.
static void undoHalt(void) {
    if (!g_Running) {
        g_Running = true;

        KYGXError ret = tryExecNextBatch();
        if (ret == KYGX_ERROR_EMPTY)
            return;

        while (ret == KYGX_ERROR_BUSY) {
            ctrYield();
            ret = tryExecNextBatch();
        }

        CTR_BREAK_IF(ret != KYGX_ERROR_SUCCESS);
    }
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

static size_t intrIdxForSyncCmd(const KYGXCmd* cmd) {
    switch (cmd->header & 0xFF) {
        case KYGX_CMD_REQUESTDMA:
            return INTR_DMA;
        case KYGX_CMD_PROCESSCOMMANDLIST:
            return INTR_P3D;
        case KYGX_CMD_DISPLAYTRANSFER:
        case KYGX_CMD_TEXTURECOPY:
            return INTR_PPF;
    }

    // When using two buffers MemoryFill only triggers PSC0.
    // TODO: this is an HOS specific quirk which should be handled in HOS/GXServer.c.
    if ((cmd->header & 0xFF) == KYGX_CMD_MEMORYFILL) {
        const bool buf0 = cmd->params[0];
        const bool buf1 = cmd->params[3];
        return buf1 && !buf0 ? INTR_PSC1 : INTR_PSC0;
    }

    return -1;
}

KYGXError kygxExecSync(const KYGXCmd* command) {
    const size_t intrIdx = intrIdxForSyncCmd(command);

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
    KYGXError ret = GXServerExec(&it);
    
    while (ret == KYGX_ERROR_BUSY) {
        ctrYield();
        ret = GXServerExec(&it);
    }

    if (ret == KYGX_ERROR_SUCCESS) {
        // Wait for termination.
        if (intrIdx != -1)
            kygxWaitIntr(intrForIndex(intrIdx));
    }

    // Resume processing.
    GXServerSetCallbacks(onInterrupt, onBatchCompleted);
    undoHalt();

    ctrMtxRelease(g_BatchMtx);

    return ret;
}