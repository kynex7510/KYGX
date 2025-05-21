#include "State.h"

#ifndef CTRGX_BAREMETAL

#define SAFE_OPS (STATEOP_INTRCB | STATEOP_INTR_READ | STATEOP_INTR_WRITE)

bool ctrgxs_init(State* state) {
    CTRGX_ASSERT(state);

    if (R_FAILED(gspInit()))
        return false;

    LightLock_Init(&state->platform.lock);
    CondVar_Init(&state->platform.completionCV);
    CondVar_Init(&state->platform.haltCV);
    state->platform.haltRequested = false;
    state->platform.halted = true;

    const u8 clientID = gspGetClientId();
    void* sharedMem = gspGetSharedMem();
    state->intrQueue = (GXIntrQueue*)((u8*)sharedMem + (0x40 * clientID));
    state->cmdQueue = (GXCmdQueue*)((u8*)sharedMem + 0x800 + (0x200 * clientID));
    return true;
}

void ctrgxs_cleanup(State* state) {
    CTRGX_ASSERT(state);

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

void ctrgxs_set_intr_cb(State* state, GXIntr intrID, InterruptCallback cb) {
    (void)state;
    gspSetEventCallback((GSPGPU_Event)intrID, (ThreadFunc)cb, (void*)intrID, false);
}

void ctrgxs_clear_intr_cb(State* state, GXIntr intrID) {
    (void)state;
    gspSetEventCallback((GSPGPU_Event)intrID, NULL, NULL, false);
}

GXIntr ctrgxs_wait_any_intr(State* state) {
    (void)state;
    return (GXIntr)gspWaitForAnyEvent();
}

void ctrgxs_wait_intr(State* state, GXIntr intrID) {
    (void)state;
    gspWaitForEvent((GXIntr)intrID, false);
}

void ctrgxs_clear_intr(State* state, GXIntr intrID) {
    (void)state;
    gspClearEvent((GXIntr)intrID);
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
    if (!state->platform.halted) {
        state->platform.haltRequested = true;

        if (wait) {
            while (!state->platform.halted)
                CondVar_Wait(&state->platform.haltCV, &state->platform.lock);
        }
    }
}

bool ctrgxs_signal_halt(State* state) {
    state->platform.halted = true;

    if (state->platform.haltRequested) {
        state->platform.haltRequested = false;    
        CondVar_Broadcast(&state->platform.haltCV);
        return false;
    }

    return true;
}

#endif // !CTRGX_BAREMETAL