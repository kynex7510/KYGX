/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <3ds.h>

#include <CTR11/Assert.h>
#include <CTR11/Unreachable.h>

#include "Interrupt.h"

static Thread g_FlushThread = NULL;
static LightEvent g_FlushEvent;
static u8 g_FlushExit = false;

static void intrHandler(void* param) { IntrCallback((KYGXIntr)param); }

static void flushHandler(void* unused) {
    while (true) {
        LightEvent_Wait(&g_FlushEvent);
        if (g_FlushExit)
            break;

        intrHandler((void*)KYGXIntr_Flush);
    }

    threadExit(0);
}

void IntrInit(void) {
    CTR_ASSERT(g_FlushThread == NULL);

    // Initialize GSP.
    Result ret = gspInit();
    if (R_FAILED(ret)) {
        CTR_UNREACHABLE("gspInit() failed with error code 0x%08lX", ret);
    }

    // Setup emulated flush stuff.
    g_FlushExit = 0;
    LightEvent_Init(&g_FlushEvent, RESET_ONESHOT);

    g_FlushThread = threadCreate(flushHandler, NULL, 8, 0x18, -1, true);
    if (!g_FlushThread) {
        CTR_UNREACHABLE("Could not create flush thread");
    }

    // Setup callbacks.
    gspSetEventCallback(GSPGPU_EVENT_VBlank0, intrHandler, (void*)KYGXIntr_PDC0, false);
    gspSetEventCallback(GSPGPU_EVENT_VBlank1, intrHandler, (void*)KYGXIntr_PDC1, false);

    // GSP triggers only 1 interrupt per command, even if both units are used.
    gspSetEventCallback(GSPGPU_EVENT_PSC0, intrHandler, (void*)KYGXIntr_PSC, false);
    gspSetEventCallback(GSPGPU_EVENT_PSC1, intrHandler, (void*)KYGXIntr_PSC, false);
    
    gspSetEventCallback(GSPGPU_EVENT_PPF, intrHandler, (void*)KYGXIntr_PPF, false);
    gspSetEventCallback(GSPGPU_EVENT_P3D, intrHandler, (void*)KYGXIntr_P3D, false);
    gspSetEventCallback(GSPGPU_EVENT_DMA, intrHandler, (void*)KYGXIntr_DMA, false);
}

void IntrExit(void) {
    CTR_ASSERT(g_FlushThread);

    // Remove callbacks.
    gspSetEventCallback(GSPGPU_EVENT_VBlank0, NULL, NULL, false);
    gspSetEventCallback(GSPGPU_EVENT_VBlank1, NULL, NULL, false);
    gspSetEventCallback(GSPGPU_EVENT_PSC0, NULL, NULL, false);
    gspSetEventCallback(GSPGPU_EVENT_PSC1, NULL, NULL, false);
    gspSetEventCallback(GSPGPU_EVENT_PPF, NULL, NULL, false);
    gspSetEventCallback(GSPGPU_EVENT_P3D, NULL, NULL, false);
    gspSetEventCallback(GSPGPU_EVENT_DMA, NULL, NULL, false);

    // Kill emulated flush stuff.
    do {
        __ldrexb(&g_FlushExit);
    } while (__strexb(&g_FlushExit, 1));

    LightEvent_Signal(&g_FlushEvent);
    threadJoin(g_FlushThread, U64_MAX);

    threadFree(g_FlushThread);
    gspExit();
}

void IntrSetPSC(bool unit0, bool unit1) { CTR_ASSERT(g_FlushThread); }

void IntrSignalFlush(void) {
    CTR_ASSERT(g_FlushThread);

    LightEvent_Signal(&g_FlushEvent);
}