#include <kevent.h>
#include <drivers/gfx.h>

#include "CmdImpl.h"

#define SAFE_OPS STATEOP_INTR
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

static void initIntrThreads(void) {
    KYGX_BREAK_UNLESS(createTask(0x200, 3, intrThread, (void*)KYGX_INTR_PSC0));
    KYGX_BREAK_UNLESS(createTask(0x200, 3, intrThread, (void*)KYGX_INTR_PSC1));
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

    state->platform.lock = createMutex();
    KYGX_ASSERT(state->platform.lock);

    CV_Init(&state->platform.completionCV);
    CV_Init(&state->platform.haltCV);
    state->platform.haltRequested = false;
    state->platform.halted = true;

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

    CV_Destroy(&state->platform.haltCV);
    CV_Destroy(&state->platform.completionCV);
    deleteMutex(state->platform.lock);

    for (size_t i = 0; i< KYGX_NUM_INTERRUPTS; ++i)
        deleteEvent(g_IntrEvents[i]);
}

void kygxs_enter_critical_section(State* state, u32 op) {
    KYGX_ASSERT(state);

    if (op & ~SAFE_OPS) {
        KYGX_BREAK_UNLESS(lockMutex(state->platform.lock) == KRES_OK);
    }
}

void kygxs_exit_critical_section(State* state, u32 op) {
    KYGX_ASSERT(state);

    if (op & ~SAFE_OPS) {
        KYGX_BREAK_UNLESS(unlockMutex(state->platform.lock) == KRES_OK);
    }
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

    state->platform.halted = false;

    KYGXCmd cmd;
    while (kygxCmdQueuePop(cmdQueue, &cmd)) {
        execCommand(&cmd);

        if (cmd.header & KYGX_CMDHEADER_FLAG_LAST)
            break;
    }

    kygxCmdQueueSetHalt(cmdQueue);
}

void kygxs_wait_command_completion(State* state) {
    KYGX_ASSERT(state);
    
    const KYGXCmdBuffer* cmdBuffer = state->cmdBuffer;
    while (cmdBuffer && cmdBuffer->count) {
        CV_Wait(&state->platform.completionCV, state->platform.lock);
        cmdBuffer = state->cmdBuffer;
    }
}

void kygxs_signal_command_completion(State* state) {
    KYGX_ASSERT(state);
    KYGX_ASSERT(!state->cmdBuffer || !state->cmdBuffer->count);
    
    CV_Broadcast(&state->platform.completionCV);
}

void kygxs_request_halt(State* state, bool wait) {
    KYGX_ASSERT(state);

    if (!state->platform.halted) {
        state->platform.haltRequested = true;

        if (wait) {
            while (!state->platform.halted)
                CV_Wait(&state->platform.haltCV, state->platform.lock);
        }
    }
}

bool kygxs_signal_halt(State* state) {
    KYGX_ASSERT(state);

    state->platform.halted = true;

    if (state->platform.haltRequested) {
        state->platform.haltRequested = false;
        CV_Broadcast(&state->platform.haltCV);
        return false;
    }

    return true;
}