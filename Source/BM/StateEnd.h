#include "CmdImpl.h"

#define SAFE_OPS STATEOP_INTR
#define INTR_MASK(index) (1 << ((u32)index))

// DMA interrupt is not supported.
#undef CTRGX_NUM_INTERRUPTS
#define CTRGX_NUM_INTERRUPTS 6

static GXCmdQueue g_CmdQueue;

static u8 g_IntrMask = 0;
static KHandle g_IntrEvents[CTRGX_NUM_INTERRUPTS];

// Defined in GX.c.
static void onInterrupt(GXIntr intrID);

// INTR HACK START

static void intrThread(void* intrID) {
    const size_t index = (size_t)intrID;
    CTRGX_ASSERT(index < CTRGX_NUM_INTERRUPTS);

    while (true) {
        GFX_waitForEvent((GfxEvent)intrID);

        if (g_IntrMask & INTR_MASK(index))
            onInterrupt((GXIntr)intrID);

        // TODO: reschedule?
        signalEvent(g_IntrEvents[index], false);
    }
}

static void initIntrThreads(void) {
    CTRGX_BREAK_UNLESS(createTask(0x200, 3, intrThread, (void*)GX_INTR_PSC0));
    CTRGX_BREAK_UNLESS(createTask(0x200, 3, intrThread, (void*)GX_INTR_PSC1));
    CTRGX_BREAK_UNLESS(createTask(0x200, 3, intrThread, (void*)GX_INTR_PDC0));
    CTRGX_BREAK_UNLESS(createTask(0x200, 3, intrThread, (void*)GX_INTR_PDC1));
    CTRGX_BREAK_UNLESS(createTask(0x200, 3, intrThread, (void*)GX_INTR_PPF));
    CTRGX_BREAK_UNLESS(createTask(0x200, 3, intrThread, (void*)GX_INTR_P3D));
}

// INTR HACK END

bool ctrgxs_init(State* state) {
    CTRGX_ASSERT(state);

    const u8 defaultIntrMask = INTR_MASK(GX_INTR_PSC0) | INTR_MASK(GX_INTR_PSC1) | INTR_MASK(GX_INTR_PPF) | INTR_MASK(GX_INTR_P3D);

    do {
        __ldrexb(&g_IntrMask);
    } while (__strexb(&g_IntrMask, defaultIntrMask));

    for (size_t i = 0; i < CTRGX_NUM_INTERRUPTS; ++i)
        g_IntrEvents[i] = createEvent(false);

    state->platform.lock = createMutex();
    CTRGX_ASSERT(state->platform.lock != NULL);

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

void ctrgxs_cleanup(State* state) {
    CTRGX_ASSERT(state);

    // TODO: intr callback

    state->intrQueue = NULL;
    state->cmdQueue = NULL;

    CV_Destroy(&state->platform.haltCV);
    CV_Destroy(&state->platform.completionCV);
    deleteMutex(state->platform.lock);

    for (size_t i = 0; i< CTRGX_NUM_INTERRUPTS; ++i)
        deleteEvent(g_IntrEvents[i]);
}

void ctrgxs_enter_critical_section(State* state, u32 op) {
    CTRGX_ASSERT(state);

    if (op & ~SAFE_OPS) {
        CTRGX_BREAK_UNLESS(lockMutex(state->platform.lock) == KRES_OK);
    }
}

void ctrgxs_exit_critical_section(State* state, u32 op) {
    CTRGX_ASSERT(state);

    if (op & ~SAFE_OPS) {
        CTRGX_BREAK_UNLESS(unlockMutex(state->platform.lock) == KRES_OK);
    }
}

void ctrgxs_enable_intr_cb(State* state, GXIntr intrID) {
    (void)state;

    const size_t index = (size_t)intrID;
    CTRGX_ASSERT(index < CTRGX_NUM_INTERRUPTS);

    u8 mask;
    do {
        mask = __ldrexb(&g_IntrMask);
    } while (__strexb(&g_IntrMask, mask | INTR_MASK(index)));
}

void ctrgxs_disable_intr_cb(State* state, GXIntr intrID) {
    (void)state;

    const size_t index = (size_t)intrID;
    CTRGX_ASSERT(index < CTRGX_NUM_INTERRUPTS);

    u8 mask;
    do {
        mask = __ldrexb(&g_IntrMask);
    } while (__strexb(&g_IntrMask, mask & ~INTR_MASK(index)));
}

void ctrgxs_wait_intr(State* state, GXIntr intrID) {
    (void)state;
    
    const size_t index = (size_t)intrID;
    CTRGX_ASSERT(index < CTRGX_NUM_INTERRUPTS);

    CTRGX_BREAK_UNLESS(waitForEvent(g_IntrEvents[index]) == KRES_OK);
}

void ctrgxs_clear_intr(State* state, GXIntr intrID) {
    (void)state;
    
    const size_t index = (size_t)intrID;
    CTRGX_ASSERT(index < CTRGX_NUM_INTERRUPTS);

    clearEvent(g_IntrEvents[index]);
}

void ctrgxs_exec_commands(State* state) {
    CTRGX_ASSERT(state);

    GXCmdQueue* cmdQueue = state->cmdQueue;
    CTRGX_ASSERT(cmdQueue);

    state->platform.halted = false;

    GXCmd cmd;
    while (ctrgxCmdQueuePop(cmdQueue, &cmd)) {
        execCommand(&cmd);

        if (cmd.header & CTRGX_CMDHEADER_FLAG_LAST)
            break;
    }

    ctrgxCmdQueueHalt(cmdQueue, true);
}

void ctrgxs_wait_command_completion(State* state) {
    CTRGX_ASSERT(state);
    
    const GXCmdBuffer* cmdBuffer = state->cmdBuffer;
    while (cmdBuffer && cmdBuffer->count) {
        CV_Wait(&state->platform.completionCV, state->platform.lock);
        cmdBuffer = state->cmdBuffer;
    }
}

void ctrgxs_signal_command_completion(State* state) {
    CTRGX_ASSERT(state);
    CTRGX_ASSERT(!state->cmdBuffer || !state->cmdBuffer->count);
    
    CV_Broadcast(&state->platform.completionCV);
}

void ctrgxs_request_halt(State* state, bool wait) {
    CTRGX_ASSERT(state);

    if (!state->platform.halted) {
        state->platform.haltRequested = true;

        if (wait) {
            while (!state->platform.halted)
                CV_Wait(&state->platform.haltCV, state->platform.lock);
        }
    }
}

bool ctrgxs_signal_halt(State* state) {
    CTRGX_ASSERT(state);

    state->platform.halted = true;

    if (state->platform.haltRequested) {
        state->platform.haltRequested = false;
        CV_Broadcast(&state->platform.haltCV);
        return false;
    }

    return true;
}