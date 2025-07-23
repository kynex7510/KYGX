/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <kevent.h>
#include <drivers/gfx.h>

// #include "../State.h"

#include <KYGX/CommandBuffer.h>
#include <KYGX/Interrupt.h>

#include "CmdImpl.h"

#define INTR_MASK(index) (1 << ((u32)index))

// DMA interrupt is not supported.
#undef KYGX_NUM_INTERRUPTS
#define KYGX_NUM_INTERRUPTS 6

static KYGXCmdQueue g_CmdQueue;

static u8 g_IntrMask = 0;
static KHandle g_IntrEvents[KYGX_NUM_INTERRUPTS];

// Defined in GX.c.
static void onInterrupt(KYGXIntr intrID);

// INTR HACK BEGIN

/*
    The following is a hack, which happens to work well enough for now.
    Currently, libn3ds provides no way of setting a callback for GPU interrupts (akin to gspSetEventCallback in libctru).
    The workaround is to spawn a thread for each interrupt. This hack is pretty bad because, other than being resource
    inefficient and wasting context switches, it's not possible to signal each thread when to terminate, as we have no
    control over the interrupt events, which are managed internally by libn3ds. This means an application shall not call
    kygxInit and kygxExit more than once in its entire lifetime.
*/

static void intrThread(void* intrID) {
    const size_t index = (size_t)intrID;
    KYGX_ASSERT(index < KYGX_NUM_INTERRUPTS);

    while (true) {
        GFX_waitForEvent((GfxEvent)intrID);

        if (g_IntrMask & INTR_MASK(index))
            onInterrupt((KYGXIntr)intrID);

        // TODO: reschedule?
        signalEvent(g_IntrEvents[index], false);
    }
}

static void intrThreadPSC0(void* unused) {
    const size_t index = (size_t)KYGX_INTR_PSC0;

    while (true) {
        GFX_waitForEvent(GFX_EVENT_PSC0);

        // Update PSC flags.
        if (g_PSCFlags & PSC_FLAG_MULTIPLE_PSC) {
            u8 flags;

            do {
                flags = __ldrexb(&g_PSCFlags) & ~PSC_FLAG_PSC0_BUSY;
            } while (__strexb(&g_PSCFlags, flags));

            // Check if we have PSC1 pending.
            if (flags & ~PSC_FLAG_MULTIPLE_PSC)
                continue;

            do {
                __ldrexb(&g_PSCFlags);
            } while (__strexb(&g_PSCFlags, 0));
        }

        if (g_IntrMask & INTR_MASK(index))
            onInterrupt(KYGX_INTR_PSC0);

        // TODO: reschedule?
        signalEvent(g_IntrEvents[index], false);
    }
}

static void intrThreadPSC1(void* unused) {
    while (true) {
        KYGXIntr toFire = KYGX_INTR_PSC1;

        GFX_waitForEvent(GFX_EVENT_PSC1);

        // Update PSC flags.
        if (g_PSCFlags & PSC_FLAG_MULTIPLE_PSC) {
            u8 flags;

            do {
                flags = __ldrexb(&g_PSCFlags) & ~PSC_FLAG_PSC1_BUSY;
            } while (__strexb(&g_PSCFlags, flags));

            // Check if we have PSC0 pending.
            if (flags & ~PSC_FLAG_MULTIPLE_PSC)
                continue;

            do {
                __ldrexb(&g_PSCFlags);
            } while (__strexb(&g_PSCFlags, 0));

            // We shall fire PSC0.
            toFire = KYGX_INTR_PSC0;
        }

        const size_t index = (size_t)toFire;

        if (g_IntrMask & INTR_MASK(index))
            onInterrupt(toFire);

        // TODO: reschedule?
        signalEvent(g_IntrEvents[index], false);
    }
}

static void initIntrThreads(void) {
    KYGX_BREAK_UNLESS(createTask(0x200, 3, intrThreadPSC0, NULL));
    KYGX_BREAK_UNLESS(createTask(0x200, 3, intrThreadPSC1, NULL));
    KYGX_BREAK_UNLESS(createTask(0x200, 3, intrThread, (void*)KYGX_INTR_PDC0));
    KYGX_BREAK_UNLESS(createTask(0x200, 3, intrThread, (void*)KYGX_INTR_PDC1));
    KYGX_BREAK_UNLESS(createTask(0x200, 3, intrThread, (void*)KYGX_INTR_PPF));
    KYGX_BREAK_UNLESS(createTask(0x200, 3, intrThread, (void*)KYGX_INTR_P3D));
}

// INTR HACK END

bool kygxs_init(State* state) {
    KYGX_ASSERT(state);

    const u8 defaultIntrMask = INTR_MASK(KYGX_INTR_PSC0) | INTR_MASK(KYGX_INTR_PSC1) | INTR_MASK(KYGX_INTR_PPF) | INTR_MASK(KYGX_INTR_P3D);

    do {
        __ldrexb(&g_IntrMask);
    } while (__strexb(&g_IntrMask, defaultIntrMask));

    for (size_t i = 0; i < KYGX_NUM_INTERRUPTS; ++i)
        g_IntrEvents[i] = createEvent(false);

    kygxLockInit(&state->lock);
    kygxCVInit(&state->completionCV);
    kygxCVInit(&state->haltCV);
    state->haltRequested = false;
    state->halted = true;
    state->intrQueue = NULL;
    state->cmdQueue = &g_CmdQueue;

    // TODO: intr callback
    initIntrThreads();
    //

    return true;
}

void kygxs_cleanup(State* state) {
    KYGX_ASSERT(state);

    // TODO: intr callback

    state->intrQueue = NULL;
    state->cmdQueue = NULL;

    kygxCVDestroy(&state->haltCV);
    kygxCVDestroy(&state->completionCV);
    kygxLockDestroy(&state->lock);

    for (size_t i = 0; i< KYGX_NUM_INTERRUPTS; ++i)
        deleteEvent(g_IntrEvents[i]);
}

void kygxs_enable_intr_cb(State* state, KYGXIntr intrID) {
    (void)state;

    const size_t index = (size_t)intrID;
    KYGX_ASSERT(index < KYGX_NUM_INTERRUPTS);

    u8 mask;
    do {
        mask = __ldrexb(&g_IntrMask);
    } while (__strexb(&g_IntrMask, mask | INTR_MASK(index)));
}

void kygxs_disable_intr_cb(State* state, KYGXIntr intrID) {
    (void)state;

    const size_t index = (size_t)intrID;
    KYGX_ASSERT(index < KYGX_NUM_INTERRUPTS);

    u8 mask;
    do {
        mask = __ldrexb(&g_IntrMask);
    } while (__strexb(&g_IntrMask, mask & ~INTR_MASK(index)));
}

void kygxs_wait_intr(State* state, KYGXIntr intrID) {
    (void)state;
    
    const size_t index = (size_t)intrID;
    KYGX_ASSERT(index < KYGX_NUM_INTERRUPTS);

    KYGX_BREAK_UNLESS(waitForEvent(g_IntrEvents[index]) == KRES_OK);
}

void kygxs_clear_intr(State* state, KYGXIntr intrID) {
    (void)state;
    
    const size_t index = (size_t)intrID;
    KYGX_ASSERT(index < KYGX_NUM_INTERRUPTS);

    clearEvent(g_IntrEvents[index]);
}

void kygxs_exec_commands(State* state) {
    KYGX_ASSERT(state);

    KYGXCmdQueue* cmdQueue = state->cmdQueue;
    KYGX_ASSERT(cmdQueue);

    state->halted = false;

    KYGXCmd cmd;
    while (kygxCmdQueuePop(cmdQueue, &cmd)) {
        execCommand(&cmd);

        if (cmd.header & KYGX_CMDHEADER_FLAG_LAST)
            break;
    }

    kygxCmdQueueSetHalt(cmdQueue);
}