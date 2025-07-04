/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _KYGX_STATE_H
#define _KYGX_STATE_H

#ifdef KYGX_BAREMETAL
#include "BM/StateBegin.h"
#else
#include "HOS/StateBegin.h"
#endif // KYGX_BAREMETAL

typedef struct {
    PlatformState platform;
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

KYGX_INLINE void kygxs_enter_critical_section(State* state, u32 op);
KYGX_INLINE void kygxs_exit_critical_section(State* state, u32 op);

KYGX_INLINE void kygxs_enable_intr_cb(State* state, KYGXIntr intrID);
KYGX_INLINE void kygxs_disable_intr_cb(State* state, KYGXIntr intrID);
KYGX_INLINE void kygxs_wait_intr(State* state, KYGXIntr intrID);
KYGX_INLINE void kygxs_clear_intr(State* state, KYGXIntr intrID);

KYGX_INLINE void kygxs_exec_commands(State* state);

// Command completion is signaled when the command buffer becomes empty.
KYGX_INLINE void kygxs_wait_command_completion(State* state);
KYGX_INLINE void kygxs_signal_command_completion(State* state);

// Halting is signaled every time a batch is completed.
KYGX_INLINE void kygxs_request_halt(State* state, bool wait);
KYGX_INLINE bool kygxs_signal_halt(State* state);

#ifdef KYGX_BAREMETAL
#include "BM/StateEnd.h"
#else
#include "HOS/StateEnd.h"
#endif // KYGX_BAREMETAL

#endif /* _KYGX_STATE_H */