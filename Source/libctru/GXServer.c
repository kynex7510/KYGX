/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <3ds.h>
#include <CTR/Unreachable.h>
#include <CTR/Assert.h>

#include "GXServer.h"

#include <string.h>

#define CMD_QUEUE_PTR(sharedMem, index) (GSPCmdQueue*)((u8*)(sharedMem) + 0x800 + (0x200 * (index)))

#define CMDQUEUE_STATUS_HALTED 0x01
#define CMDQUEUE_STATUS_ERRORED 0x80

#define CMDHEADER_FLAG_LAST (1 << 16)

#define MAX_CMDS_IN_QUEUE 15

typedef struct {
    u8 index;
    u8 count;
    u8 status;
    u8 requestHalt;
    s32 lastError;
    u8 _pad[0x18];
    KYGXCmd list[MAX_CMDS_IN_QUEUE];
} GSPCmdQueue;

static void* g_SharedMem = NULL;
static u8 g_ClientIndex = -1;
static GSPCmdQueue* g_CmdQueue = NULL;

static GXOnInterrupt g_UserOnInterrupt = NULL;
static GXOnBatchCompleted g_UserOnBatchCompleted = NULL;

static u8 g_NumPendingCommands = 0;

static u8 detectClientIndex(void* sharedMem) {
    u32 tmp[2];
    u32* src = tmp;
    u32* dst = &tmp[1];
    const u32 size = sizeof(u32);
    GX_RequestDma(src, dst, size);
    gspWaitForDMA();

    for (size_t i = 0; i < 4; ++i) {
        const GSPCmdQueue* q = CMD_QUEUE_PTR(sharedMem, i);
        const KYGXCmd* cmd = &q->list[(q->index - 1) % MAX_CMDS_IN_QUEUE];

        if ((cmd->header & 0xFF) == KYGX_CMD_REQUESTDMA &&
            cmd->params[0] == (u32)src &&
            cmd->params[1] == (u32)dst &&
            cmd->params[2] == (u32)size)
            return i;
    }

    return 0xFF;
}

static void* detectSharedMem(u8* outIndex) {
    MemInfo memInfo;
    PageInfo pageInfo;
    u32 addr = OS_MAP_AREA_BEGIN;

    while (addr < OS_MAP_AREA_END) {
        if (R_FAILED(svcQueryMemory(&memInfo, &pageInfo, addr)))
            break;

        if (memInfo.size == 0x1000 && memInfo.perm == MEMPERM_READWRITE && memInfo.state == MEMSTATE_SHARED) {
            void* sharedMem = (void*)memInfo.base_addr;
            const u8 index = detectClientIndex(sharedMem);
            if (index != 0xFF) {
                *outIndex = index;
                return sharedMem;
            }
        }

        addr = memInfo.base_addr + memInfo.size;
    }

    return NULL;
}

static KYGXIntr getIntrID(void* param) {
    const size_t intr = (size_t)param;
    switch ((KYGXIntr)intr) {
        case KYGX_INTR_PSC0:
        case KYGX_INTR_PSC1:
        case KYGX_INTR_PDC0:
        case KYGX_INTR_PDC1:
        case KYGX_INTR_PPF:
        case KYGX_INTR_P3D:
        case KYGX_INTR_DMA:
            return (KYGXIntr)intr;
        default:
            CTR_UNREACHABLE("Unknown interrupt %u", intr);
    }
}

static void onInterrupt(void* param) {
    const KYGXIntr intrID = getIntrID(param);
    
    if (g_UserOnInterrupt)
        g_UserOnInterrupt(intrID);

    // Nothing more to do for PDC0, PDC1.
    if (intrID == KYGX_INTR_PDC0 || intrID == KYGX_INTR_PDC1)
        return;

    // We should not be getting spurious interrupts.
    CTR_ASSERT(g_NumPendingCommands > 0);

    // Update state.
    --g_NumPendingCommands;

    // Handle batch termination.
    if (--g_NumPendingCommands) {
        // It's possible that, at this point, GSP still hasn't halted.
        CTR_ASSERT(g_CmdQueue);
        CTR_ASSERT(!g_CmdQueue->count);

        while (g_CmdQueue->status != CMDQUEUE_STATUS_HALTED) {
            svcSleepThread(0);
            CTR_BREAK_IF(g_CmdQueue->status == CMDQUEUE_STATUS_ERRORED);
        }

        // Invoke callback.
        if (g_UserOnBatchCompleted)
            g_UserOnBatchCompleted();
    }
}

KYGXError GXServerInit(void) {
    CTR_ASSERT(g_SharedMem == NULL);

    // Initialize GSP.
    if (R_FAILED(gspInit()))
        return KYGX_ERROR_SYSTEM;

    g_SharedMem = detectSharedMem(&g_ClientIndex);
    if (!g_SharedMem) {
        gspExit();
        return KYGX_ERROR_SYSTEM;
    }

    g_CmdQueue = CMD_QUEUE_PTR(g_SharedMem, g_ClientIndex);

    // Set queue status to halted.
    do {
        __ldrexb(&g_CmdQueue->status);
    } while (__strexb(&g_CmdQueue->status, CMDQUEUE_STATUS_HALTED));

    // Initialize state.
    g_UserOnInterrupt = NULL;
    g_UserOnBatchCompleted = NULL;
    g_NumPendingCommands = 0;

    // Setup callbacks.
    gspSetEventCallback(GSPGPU_EVENT_PSC0, onInterrupt, (void*)KYGX_INTR_PSC0, false);
    gspSetEventCallback(GSPGPU_EVENT_PSC1, onInterrupt, (void*)KYGX_INTR_PSC1, false);
    gspSetEventCallback(GSPGPU_EVENT_VBlank0, onInterrupt, (void*)KYGX_INTR_PDC0, false);
    gspSetEventCallback(GSPGPU_EVENT_VBlank1, onInterrupt, (void*)KYGX_INTR_PDC1, false);
    gspSetEventCallback(GSPGPU_EVENT_PPF, onInterrupt, (void*)KYGX_INTR_PPF, false);
    gspSetEventCallback(GSPGPU_EVENT_P3D, onInterrupt, (void*)KYGX_INTR_P3D, false);
    gspSetEventCallback(GSPGPU_EVENT_DMA, onInterrupt, (void*)KYGX_INTR_DMA, false);

    return KYGX_ERROR_SUCCESS;
}

void GXServerExit(void) {
    CTR_ASSERT(g_SharedMem);

    // Wait command execution.
    CTR_ASSERT(g_CmdQueue);
    if (g_CmdQueue->count) {
        while (g_CmdQueue->status != CMDQUEUE_STATUS_HALTED) {
            svcSleepThread(0);
            CTR_BREAK_IF(g_CmdQueue->status == CMDQUEUE_STATUS_ERRORED);
        }
    }

    gspSetEventCallback(GSPGPU_EVENT_PSC0, NULL, NULL, false);
    gspSetEventCallback(GSPGPU_EVENT_PSC1, NULL, NULL, false);
    gspSetEventCallback(GSPGPU_EVENT_VBlank0, NULL, NULL, false);
    gspSetEventCallback(GSPGPU_EVENT_VBlank1, NULL, NULL, false);
    gspSetEventCallback(GSPGPU_EVENT_PPF, NULL, NULL, false);
    gspSetEventCallback(GSPGPU_EVENT_P3D, NULL, NULL, false);
    gspSetEventCallback(GSPGPU_EVENT_DMA, NULL, NULL, false);

    // Clear queue status.
    do {
        __ldrexb(&g_CmdQueue->status);
    } while (__strexb(&g_CmdQueue->status, 0));

    g_SharedMem = NULL;
    g_ClientIndex = -1;
    g_CmdQueue = NULL;

    gspExit();
    return KYGX_ERROR_SUCCESS;
}

void GXServerSetCallbacks(GXOnInterrupt onInterrupt, GXOnBatchCompleted onBatchCompleted) {
    g_UserOnInterrupt = onInterrupt;
    g_UserOnBatchCompleted = onBatchCompleted;
}

static void addCommandToQueue(const KYGXCmd* cmd) {
    CTR_ASSERT(g_CmdQueue);
    CTR_ASSERT(g_CmdQueue->count < MAX_CMDS_IN_QUEUE);

    u32 header;

    do {
        header = __ldrex((u32*)g_CmdQueue);
        const u8 count = (header >> 8) & 0xFF;
        const u8 index = (count + (header & 0xFF)) % 15;

        memcpy(&g_CmdQueue->list[index], cmd, sizeof(KYGXCmd));
         __dsb();

        header = (header & 0xFFFF00FF) | ((count + 1) << 8);
    } while (__strex((u32*)g_CmdQueue, header));
}

static inline KYGXError triggerCommandHandling(void) {
    CTR_ASSERT(g_CmdQueue);

    // Clear status.
    do {
        __ldrexb(&g_CmdQueue->status);
    } while (__strexb(&g_CmdQueue->status, 0));

    // Execute commands.
    return R_SUCCEEDED(GSPGPU_TriggerCmdReqQueue()) ? KYGX_ERROR_SUCCESS : KYGX_ERROR_SYSTEM;
}

KYGXError GXServerExec(CmdIterator* it) {
    CTR_ASSERT(g_SharedMem);
    CTR_ASSERT(it);

    // Check if we can add commands.
    size_t numCommands = CmdIteratorCount(it);

    if (!numCommands)
        return KYGX_ERROR_EMPTY;

    if (numCommands > MAX_CMDS_IN_QUEUE)
        return KYGX_ERROR_NO_MEM;

    if (g_CmdQueue->status != CMDQUEUE_STATUS_HALTED)
        return KYGX_ERROR_BUSY;

    // Add commands.
    for (size_t i = 0; i < numCommands; ++i) {
        KYGXCmd tmp;
        const KYGXCmd* src = CmdIteratorNext(it);
        CTR_ASSERT(src);

        memcpy(&tmp, src, sizeof(KYGXCmd));

        if (i == (numCommands - 1))
            tmp.header |= CMDHEADER_FLAG_LAST;

        addCommandToQueue(&tmp);
    }

    g_NumPendingCommands = numCommands;

    // Execute commands.
    return triggerCommandHandling();
}