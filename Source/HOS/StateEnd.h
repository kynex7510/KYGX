/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "../State.h"

#define SAFE_OPS STATEOP_INTR

#define INTR_QUEUE_PTR(sharedMem, index) (KYGXIntrQueue*)((u8*)(sharedMem) + (0x40 * (index)))
#define CMD_QUEUE_PTR(sharedMem, index) (KYGXCmdQueue*)((u8*)(sharedMem) + 0x800 + (0x200 * (index)))

static u8 g_IntrMask = 0;
static LightEvent g_IntrEvents[KYGX_NUM_INTERRUPTS];

// Defined in GX.c.
static void onInterrupt(KYGXIntr intrID);

static void gspIntrCb(void* intrID) {
    const size_t index = (size_t)intrID;
    KYGX_ASSERT(index < KYGX_NUM_INTERRUPTS);

    if (g_IntrMask & (1 << index))
        onInterrupt((KYGXIntr)intrID);

    LightEvent_Signal(&g_IntrEvents[index]);
}

bool kygxs_init(State* state) {
    KYGX_ASSERT(state);

    if (R_FAILED(gspInit()))
        return false;

    do {
        __ldrexb(&g_IntrMask);
    } while (__strexb(&g_IntrMask, 0xFF));

    for (size_t i = 0; i < KYGX_NUM_INTERRUPTS; ++i)
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

    gspSetEventCallback(GSPGPU_EVENT_PSC0, gspIntrCb, (void*)KYGX_INTR_PSC0, false);
    gspSetEventCallback(GSPGPU_EVENT_PSC1, gspIntrCb, (void*)KYGX_INTR_PSC1, false);
    gspSetEventCallback(GSPGPU_EVENT_VBlank0, gspIntrCb, (void*)KYGX_INTR_PDC0, false);
    gspSetEventCallback(GSPGPU_EVENT_VBlank1, gspIntrCb, (void*)KYGX_INTR_PDC1, false);
    gspSetEventCallback(GSPGPU_EVENT_PPF, gspIntrCb, (void*)KYGX_INTR_PPF, false);
    gspSetEventCallback(GSPGPU_EVENT_P3D, gspIntrCb, (void*)KYGX_INTR_P3D, false);
    gspSetEventCallback(GSPGPU_EVENT_DMA, gspIntrCb, (void*)KYGX_INTR_DMA, false);
    return true;
}

void kygxs_cleanup(State* state) {
    KYGX_ASSERT(state);

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

void kygxs_enter_critical_section(State* state, u32 op) {
    KYGX_ASSERT(state);

    if (op & ~SAFE_OPS)
        LightLock_Lock(&state->platform.lock);
}

void kygxs_exit_critical_section(State* state, u32 op) {
    KYGX_ASSERT(state);

    if (op & ~SAFE_OPS)
        LightLock_Unlock(&state->platform.lock);
}

void kygxs_enable_intr_cb(State* state, KYGXIntr intrID) {
    (void)state;

    const size_t index = (size_t)intrID;
    KYGX_ASSERT(index < KYGX_NUM_INTERRUPTS);

    u8 mask;
    do {
        mask = __ldrexb(&g_IntrMask);
    } while (__strexb(&g_IntrMask, mask | (1 << index)));
}

void kygxs_disable_intr_cb(State* state, KYGXIntr intrID) {
    (void)state;

    const size_t index = (size_t)intrID;
    KYGX_ASSERT(index < KYGX_NUM_INTERRUPTS);

    u8 mask;
    do {
        mask = __ldrexb(&g_IntrMask);
    } while (__strexb(&g_IntrMask, mask & ~(1 << index)));
}

void kygxs_wait_intr(State* state, KYGXIntr intrID) {
    (void)state;
    
    const size_t index = (size_t)intrID;
    KYGX_ASSERT(index < KYGX_NUM_INTERRUPTS);

    LightEvent_Wait(&g_IntrEvents[index]);
}

void kygxs_clear_intr(State* state, KYGXIntr intrID) {
    (void)state;
    
    const size_t index = (size_t)intrID;
    KYGX_ASSERT(index < KYGX_NUM_INTERRUPTS);

    LightEvent_Clear(&g_IntrEvents[index]);
}

void kygxs_exec_commands(State* state) {
    KYGX_ASSERT(state);
    state->platform.halted = false;

    LightLock_Unlock(&state->platform.lock);
    KYGX_BREAK_UNLESS(R_SUCCEEDED(GSPGPU_TriggerCmdReqQueue()));
    LightLock_Lock(&state->platform.lock);
}

void kygxs_wait_command_completion(State* state) {
    KYGX_ASSERT(state);
    
    const KYGXCmdBuffer* cmdBuffer = state->cmdBuffer;
    while (cmdBuffer && cmdBuffer->count) {
        CondVar_Wait(&state->platform.completionCV, &state->platform.lock);
        cmdBuffer = state->cmdBuffer;
    }
}

void kygxs_signal_command_completion(State* state) {
    KYGX_ASSERT(state);
    KYGX_ASSERT(!state->cmdBuffer || !state->cmdBuffer->count);

    CondVar_Broadcast(&state->platform.completionCV);
}

void kygxs_request_halt(State* state, bool wait) {
    KYGX_ASSERT(state);

    if (!state->platform.halted) {
        state->platform.haltRequested = true;

        if (wait) {
            while (!state->platform.halted)
                CondVar_Wait(&state->platform.haltCV, &state->platform.lock);
        }
    }
}

bool kygxs_signal_halt(State* state) {
    KYGX_ASSERT(state);

    state->platform.halted = true;

    if (state->platform.haltRequested) {
        state->platform.haltRequested = false;    
        CondVar_Broadcast(&state->platform.haltCV);
        return false;
    }

    return true;
}