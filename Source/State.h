/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _KYGX_STATE_H
#define _KYGX_STATE_H

#include <KYGX/Sync.h>

typedef struct {
    KYGXLock lock;
    KYGXCV completionCV;
    KYGXCV haltCV;
    bool haltRequested;
    bool halted;
    KYGXIntrQueue* intrQueue;
    KYGXCmdQueue* cmdQueue;
    KYGXCmdBuffer* cmdBuffer;
    KYGXCallback currentCb;
    void* currentCbData;
    u8 pendingCommands;
    u8 completedCommands;
} State;

typedef enum {
    STATEOP_FIELD_ACCESS = 0x01, // State field access
    STATEOP_INTR = 0x02, // enable_intr_cb, disable_intr_cb, wait_intr, clear_intr
    STATEOP_EXEC_COMMANDS = 0x04, // exec_commands
    STATEOP_COMMAND_COMPLETION = 0x08, // wait_command_completion, signal_command_completion
    STATEOP_HALT = 0x10, // halt, handle_halt_request
} StateOp;

KYGX_INLINE bool kygxs_init(State* state);
KYGX_INLINE void kygxs_cleanup(State* state);

KYGX_INLINE void kygxs_enter_critical_section(State* state, u32 op) {
    KYGX_ASSERT(state);

    // Interrupt operations are safe.
    if (op != STATEOP_INTR)
        kygxLockAcquire(&state->lock);
}

KYGX_INLINE void kygxs_exit_critical_section(State* state, u32 op) {
    KYGX_ASSERT(state);

    if (op != STATEOP_INTR)
        kygxLockRelease(&state->lock);
}

KYGX_INLINE void kygxs_enable_intr_cb(State* state, KYGXIntr intrID);
KYGX_INLINE void kygxs_disable_intr_cb(State* state, KYGXIntr intrID);
KYGX_INLINE void kygxs_wait_intr(State* state, KYGXIntr intrID);
KYGX_INLINE void kygxs_clear_intr(State* state, KYGXIntr intrID);

KYGX_INLINE void kygxs_exec_commands(State* state);

// Command completion is signaled when the command buffer becomes empty.
KYGX_INLINE void kygxs_wait_command_completion(State* state) {
    KYGX_ASSERT(state);
    
    const KYGXCmdBuffer* cmdBuffer = state->cmdBuffer;
    while (cmdBuffer && cmdBuffer->count) {
        kygxCVWait(&state->completionCV, &state->lock);
        cmdBuffer = state->cmdBuffer;
    }
}

KYGX_INLINE void kygxs_signal_command_completion(State* state) {
    KYGX_ASSERT(state);
    KYGX_ASSERT(!state->cmdBuffer || !state->cmdBuffer->count);
    
    kygxCVBroadcast(&state->completionCV);
}

// Halting is signaled every time a batch is completed.
KYGX_INLINE void kygxs_request_halt(State* state, bool wait) {
    KYGX_ASSERT(state);

    if (!state->halted) {
        state->haltRequested = true;

        if (wait) {
            while (!state->halted)
                kygxCVWait(&state->haltCV, &state->lock);
        }
    }
}

KYGX_INLINE bool kygxs_signal_halt(State* state) {
    KYGX_ASSERT(state);

    state->halted = true;

    if (state->haltRequested) {
        state->haltRequested = false;
        kygxCVBroadcast(&state->haltCV);
        return false;
    }

    return true;
}

#ifdef KYGX_BAREMETAL
#include "BM/PlatformState.h"
#else
#include "HOS/PlatformState.h"
#endif // KYGX_BAREMETAL

#endif /* _KYGX_STATE_H */