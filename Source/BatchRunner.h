/**
 * @file BatchRunner.h
 * @brief Interface for executing GX command batches.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_KYGX_BATCHRUNNER_H
#define GUARD_KYGX_BATCHRUNNER_H

#include <KYGX/GX.h>

#include "BatchQueue.h"

typedef void (*RunnerOnInterrupt)(KYGXIntr intrID);
typedef void (*RunnerOnBatchCompleted)(void);

typedef enum {
    ExecState_Success,
    ExecState_NoCommands,
    ExecState_NoMemory,
    ExecState_Busy,
} ExecState;

// Initialize runner.
void BatchRunnerInit(void);

// Finalize runner.
void BatchRunnerExit(void);

// Set executor callbacks.
void BatchRunnerSetCallbacks(RunnerOnInterrupt onInterrupt, RunnerOnBatchCompleted onBatchCompleted);

// Execute batches.
ExecState BatchRunnerExec(CmdIterator* it);

#endif /* GUARD_KYGX_BATCHRUNNER_H */