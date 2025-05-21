#ifndef _CTRGX_STATE_H
#define _CTRGX_STATE_H

#include "GX/GX.h"

typedef void (*InterruptCallback)(GXIntr intrID);

typedef struct {
#ifdef CTRGX_BAREMETAL
    // ...
#else
    LightLock lock;
    CondVar completionCV;
    CondVar haltCV;
    bool haltRequested;
    bool halted;
#endif // CTRGX_BAREMETAL
} PlatformState;

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
    STATEOP_INTRCB = 0x02, // set_intr_cb, clear_intr_cb
    STATEOP_INTR_READ = 0x04, // wait_intr, wait_any_intr
    STATEOP_INTR_WRITE = 0x08, // clear_intr
    STATEOP_EXEC_COMMANDS = 0x10, // exec_commands
    STATEOP_COMMAND_COMPLETION = 0x20, // wait_command_completion, signal_command_completion
    STATEOP_HALT = 0x40, // halt, handle_halt_request
} StateOp;

CTRGX_EXTERN bool ctrgxs_init(State* state);
CTRGX_EXTERN void ctrgxs_cleanup(State* state);

CTRGX_EXTERN void ctrgxs_enter_critical_section(State* state, u32 op);
CTRGX_EXTERN void ctrgxs_exit_critical_section(State* state, u32 op);

CTRGX_EXTERN void ctrgxs_set_intr_cb(State* state, GXIntr intrID, InterruptCallback cb);
CTRGX_EXTERN void ctrgxs_clear_intr_cb(State* state, GXIntr intrID);

CTRGX_EXTERN GXIntr ctrgxs_wait_any_intr(State* state);
CTRGX_EXTERN void ctrgxs_wait_intr(State* state, GXIntr intrID);
CTRGX_EXTERN void ctrgxs_clear_intr(State* state, GXIntr intrID);

CTRGX_EXTERN void ctrgxs_exec_commands(State* state);

// Command completion is signaled when the command buffer becomes empty.
CTRGX_EXTERN void ctrgxs_wait_command_completion(State* state);
CTRGX_EXTERN void ctrgxs_signal_command_completion(State* state);

// Halting is signaled every time a batch is completed.
CTRGX_EXTERN void ctrgxs_request_halt(State* state, bool wait);
CTRGX_EXTERN bool ctrgxs_signal_halt(State* state);

#endif /* _CTRGX_STATE_H */