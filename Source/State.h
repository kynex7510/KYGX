#ifndef _CTRGX_STATE_H
#define _CTRGX_STATE_H

#ifdef CTRGX_BAREMETAL
#include "BM/StateBegin.h"
#else
#include "HOS/StateBegin.h"
#endif // CTRGX_BAREMETAL

typedef struct {
    PlatformState platform;
    GXIntrQueue* intrQueue;
    GXCmdQueue* cmdQueue;
    GXCmdBuffer* cmdBuffer;
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

CTRGX_INLINE bool ctrgxs_init(State* state);
CTRGX_INLINE void ctrgxs_cleanup(State* state);

CTRGX_INLINE void ctrgxs_enter_critical_section(State* state, u32 op);
CTRGX_INLINE void ctrgxs_exit_critical_section(State* state, u32 op);

CTRGX_INLINE void ctrgxs_enable_intr_cb(State* state, GXIntr intrID);
CTRGX_INLINE void ctrgxs_disable_intr_cb(State* state, GXIntr intrID);
CTRGX_INLINE void ctrgxs_wait_intr(State* state, GXIntr intrID);
CTRGX_INLINE void ctrgxs_clear_intr(State* state, GXIntr intrID);

CTRGX_INLINE void ctrgxs_exec_commands(State* state);

// Command completion is signaled when the command buffer becomes empty.
CTRGX_INLINE void ctrgxs_wait_command_completion(State* state);
CTRGX_INLINE void ctrgxs_signal_command_completion(State* state);

// Halting is signaled every time a batch is completed.
CTRGX_INLINE void ctrgxs_request_halt(State* state, bool wait);
CTRGX_INLINE bool ctrgxs_signal_halt(State* state);

#ifdef CTRGX_BAREMETAL
#include "BM/StateEnd.h"
#else
#include "HOS/StateEnd.h"
#endif // CTRGX_BAREMETAL

#endif /* _CTRGX_STATE_H */