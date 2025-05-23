#include "../State.h"

#define SAFE_OPS STATEOP_INTR

#define INTR_QUEUE_PTR(sharedMem, index) (GXIntrQueue*)((u8*)(sharedMem) + (0x40 * (index)))
#define CMD_QUEUE_PTR(sharedMem, index) (GXCmdQueue*)((u8*)(sharedMem) + 0x800 + (0x200 * (index)))

static u8 g_IntrMask = 0;
static LightEvent g_IntrEvents[CTRGX_NUM_INTERRUPTS];

// Defined in GX.c.
static void onInterrupt(GXIntr intrID);

static void gspIntrCb(void* intrID) {
    const size_t index = (size_t)intrID;

    if (g_IntrMask & (1 << index))
        onInterrupt((GXIntr)intrID);

    LightEvent_Signal(&g_IntrEvents[index]);
}

bool ctrgxs_init(State* state) {
    CTRGX_ASSERT(state);

    if (R_FAILED(gspInit()))
        return false;

    g_IntrMask = 0xFF;
    for (size_t i = 0; i < CTRGX_NUM_INTERRUPTS; ++i)
        LightEvent_Init(&g_IntrEvents[i], RESET_STICKY);

    LightLock_Init(&state->platform.lock);
    CondVar_Init(&state->platform.completionCV);
    CondVar_Init(&state->platform.haltCV);
    state->platform.haltRequested = false;
    state->platform.halted = true;

    void* sharedMem = gspGetSharedMem();
    const u8 index = gspGetClientId();

    state->intrQueue = INTR_QUEUE_PTR(sharedMem, index);
    state->cmdQueue = CMD_QUEUE_PTR(sharedMem, index);

    gspSetEventCallback(GSPGPU_EVENT_PSC0, gspIntrCb, (void*)GX_INTR_PSC0, false);
    gspSetEventCallback(GSPGPU_EVENT_PSC1, gspIntrCb, (void*)GX_INTR_PSC1, false);
    gspSetEventCallback(GSPGPU_EVENT_VBlank0, gspIntrCb, (void*)GX_INTR_PDC0, false);
    gspSetEventCallback(GSPGPU_EVENT_VBlank1, gspIntrCb, (void*)GX_INTR_PDC1, false);
    gspSetEventCallback(GSPGPU_EVENT_PPF, gspIntrCb, (void*)GX_INTR_PPF, false);
    gspSetEventCallback(GSPGPU_EVENT_P3D, gspIntrCb, (void*)GX_INTR_P3D, false);
    gspSetEventCallback(GSPGPU_EVENT_DMA, gspIntrCb, (void*)GX_INTR_DMA, false);
    return true;
}

void ctrgxs_cleanup(State* state) {
    CTRGX_ASSERT(state);

    gspSetEventCallback(GSPGPU_EVENT_PSC0, NULL, NULL, false);
    gspSetEventCallback(GSPGPU_EVENT_PSC1, NULL, NULL, false);
    gspSetEventCallback(GSPGPU_EVENT_VBlank0, NULL, NULL, false);
    gspSetEventCallback(GSPGPU_EVENT_VBlank1, NULL, NULL, false);
    gspSetEventCallback(GSPGPU_EVENT_PPF, NULL, NULL, false);
    gspSetEventCallback(GSPGPU_EVENT_P3D, NULL, NULL, false);
    gspSetEventCallback(GSPGPU_EVENT_DMA, NULL, NULL, false);

    state->intrQueue = NULL;
    state->cmdQueue = NULL;
    gspExit();
}

void ctrgxs_enter_critical_section(State* state, u32 op) {
    CTRGX_ASSERT(state);

    if (op & ~SAFE_OPS)
        LightLock_Lock(&state->platform.lock);
}

void ctrgxs_exit_critical_section(State* state, u32 op) {
    CTRGX_ASSERT(state);

    if (op & ~SAFE_OPS)
        LightLock_Unlock(&state->platform.lock);
}

void ctrgxs_enable_intr_cb(State* state, GXIntr intrID) {
    (void)state;

    const size_t index = (size_t)intrID;
    CTRGX_ASSERT(index < CTRGX_NUM_INTERRUPTS);

    u8 mask;
    do {
        mask = __ldrexb(&g_IntrMask);
    } while (__strexb(&g_IntrMask, mask | (1 << index)));
}

void ctrgxs_disable_intr_cb(State* state, GXIntr intrID) {
    (void)state;

    const size_t index = (size_t)intrID;
    CTRGX_ASSERT(index < CTRGX_NUM_INTERRUPTS);

    u8 mask;
    do {
        mask = __ldrexb(&g_IntrMask);
    } while (__strexb(&g_IntrMask, mask & ~(1 << index)));
}

void ctrgxs_wait_intr(State* state, GXIntr intrID) {
    (void)state;
    
    const size_t index = (size_t)intrID;
    CTRGX_ASSERT(index < CTRGX_NUM_INTERRUPTS);

    LightEvent_Wait(&g_IntrEvents[index]);
}

void ctrgxs_clear_intr(State* state, GXIntr intrID) {
    (void)state;
    
    const size_t index = (size_t)intrID;
    CTRGX_ASSERT(index < CTRGX_NUM_INTERRUPTS);

    LightEvent_Clear(&g_IntrEvents[index]);
}

void ctrgxs_exec_commands(State* state) {
    CTRGX_ASSERT(state);
    state->platform.halted = false;

    LightLock_Unlock(&state->platform.lock);
    CTRGX_BREAK_UNLESS(R_SUCCEEDED(GSPGPU_TriggerCmdReqQueue()));
    LightLock_Lock(&state->platform.lock);
}

void ctrgxs_wait_command_completion(State* state) {
    CTRGX_ASSERT(state);
    
    const GXCmdBuffer* cmdBuffer = state->cmdBuffer;
    while (cmdBuffer && cmdBuffer->count) {
        CondVar_Wait(&state->platform.completionCV, &state->platform.lock);
        cmdBuffer = state->cmdBuffer;
    }
}

void ctrgxs_signal_command_completion(State* state) {
    CTRGX_ASSERT(state);
    CTRGX_ASSERT(!state->cmdBuffer || !state->cmdBuffer->count);

    CondVar_Broadcast(&state->platform.completionCV);
}

void ctrgxs_request_halt(State* state, bool wait) {
    CTRGX_ASSERT(state);

    if (!state->platform.halted) {
        state->platform.haltRequested = true;

        if (wait) {
            while (!state->platform.halted)
                CondVar_Wait(&state->platform.haltCV, &state->platform.lock);
        }
    }
}

bool ctrgxs_signal_halt(State* state) {
    CTRGX_ASSERT(state);

    state->platform.halted = true;

    if (state->platform.haltRequested) {
        state->platform.haltRequested = false;    
        CondVar_Broadcast(&state->platform.haltCV);
        return false;
    }

    return true;
}