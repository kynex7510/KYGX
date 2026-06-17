/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <3ds.h>

#include <CTR11/Assert.h>
#include <CTR11/Unreachable.h>
#include <CTR11/Break.h>

#include "CmdQueue.h"
#include "Interrupt.h"

#include <string.h>

#define CMDQUEUE_PTR(sharedMem, index) (GSPCmdQueue*)((u8*)(sharedMem) + 0x800 + (0x200 * (index)))

#define CMDQUEUE_STATUS_HALTED 0x01
#define CMDQUEUE_STATUS_ERRORED 0x80

#define CMDHEADER_FLAG_LAST (1 << 16)

typedef struct {
    u8 index;
    u8 count;
    u8 status;
    u8 requestHalt;
    s32 lastError;
    u8 _pad[0x18];
    KYGXCmd list[CMDQUEUE_MAX_CMDS];
} GSPCmdQueue;

static_assert(CMDQUEUE_MAX_CMDS == 15);

static void* g_SharedMem = NULL;
static u8 g_ClientIndex = -1;
static GSPCmdQueue* g_CmdQueue;

//
static void intrHandler(void* param) { IntrCallback((KYGXIntr)param); }
//

static u8 detectClientIndex(void* sharedMem) {
    // Dirty hack.
    gspSetEventCallback(GSPGPU_EVENT_DMA, NULL, NULL, false);
    //

    u32 tmp[2];
    u32* src = tmp;
    u32* dst = &tmp[1];
    const u32 size = sizeof(u32);
    GX_RequestDma(src, dst, size);
    gspWaitForDMA();

    //
    gspSetEventCallback(GSPGPU_EVENT_DMA, intrHandler, (void*)KYGXIntr_DMA, false);
    //

    for (size_t i = 0; i < 4; ++i) {
        const GSPCmdQueue* q = CMDQUEUE_PTR(sharedMem, i);
        const KYGXCmd* cmd = &q->list[(q->index - 1) % CMDQUEUE_MAX_CMDS];

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

void CmdQueueInit(void) {
    CTR_ASSERT(g_SharedMem == NULL);

    // Initialize GSP.
    Result ret = gspInit();
    if (R_FAILED(ret)) {
        CTR_UNREACHABLE("gspInit() failed with error code 0x%08lX", ret);
    }

    g_SharedMem = detectSharedMem(&g_ClientIndex);
    if (!g_SharedMem) {
        gspExit();
        CTR_UNREACHABLE("Could not detect GSP shared memory");
    }

    g_CmdQueue = CMDQUEUE_PTR(g_SharedMem, g_ClientIndex);

    // Set queue status to halted.
    do {
        __ldrexb(&g_CmdQueue->status);
    } while (__strexb(&g_CmdQueue->status, CMDQUEUE_STATUS_HALTED));
}

void CmdQueueExit(void) {
    CTR_ASSERT(g_SharedMem);

    // Wait command execution.
    CTR_ASSERT(g_CmdQueue);
    if (g_CmdQueue->count) {
        while (g_CmdQueue->status != CMDQUEUE_STATUS_HALTED) {
            svcSleepThread(0);
            CTR_BREAK_IF(g_CmdQueue->status == CMDQUEUE_STATUS_ERRORED);
        }
    }

    // Clear queue status.
    do {
        __ldrexb(&g_CmdQueue->status);
    } while (__strexb(&g_CmdQueue->status, 0));

    g_SharedMem = NULL;
    g_ClientIndex = -1;
    g_CmdQueue = NULL;

    gspExit();
}

bool CmdQueueIsBusy(void) {
    CTR_ASSERT(g_CmdQueue);

    if (g_CmdQueue->status != CMDQUEUE_STATUS_HALTED) {
        CTR_BREAK_IF(g_CmdQueue->status == CMDQUEUE_STATUS_ERRORED);
        return true;
    }

    return false;
}

void CmdQueueAdd(const KYGXCmd* cmd) {
    CTR_ASSERT(g_CmdQueue);
    CTR_ASSERT(g_CmdQueue->count < CMDQUEUE_MAX_CMDS);

    u32 header;

    do {
        header = __ldrex((s32*)g_CmdQueue);
        const u8 count = (header >> 8) & 0xFF;
        const u8 index = (count + (header & 0xFF)) % CMDQUEUE_MAX_CMDS;

        memcpy(&g_CmdQueue->list[index], cmd, sizeof(KYGXCmd));
         __dsb();

        header = (header & 0xFFFF00FF) | ((count + 1) << 8);
    } while (__strex((s32*)g_CmdQueue, header));
}

void CmdQueueTriggerHandling(void) {
    CTR_ASSERT(g_CmdQueue);

    // Set final command as last.
    if (!g_CmdQueue->count)
        return;

    u32 header;
    const size_t lastIndex = (g_CmdQueue->index + g_CmdQueue->count - 1) % CMDQUEUE_MAX_CMDS;
    u32* headerPtr = &g_CmdQueue->list[lastIndex].header;

    do {
        header = __ldrex((s32*)headerPtr);
    } while (__strex((s32*)headerPtr, header | CMDHEADER_FLAG_LAST));

    // Clear status.
    do {
        __ldrexb(&g_CmdQueue->status);
    } while (__strexb(&g_CmdQueue->status, 0));

    // Execute commands.
    Result ret = GSPGPU_TriggerCmdReqQueue();
    if (R_FAILED(ret)) {
        CTR_UNREACHABLE("GSPGPU_TriggerCmdReqQueue() failed with error code 0x%08lX", ret);
    }
}