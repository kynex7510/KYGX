/**
 * @file CmdQueue.h
 * @brief Generic interface for a GX command executor implementation.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_KYGX_CMDQUEUE_H
#define GUARD_KYGX_CMDQUEUE_H

#include <KYGX/GX.h>

#define CMDQUEUE_MAX_CMDS 15

// Initialize command queue.
void CmdQueueInit(void);

// Finalize command queue.
void CmdQueueExit(void);

// Check if command queue is busy.
bool CmdQueueIsBusy(void);

// Add command.
void CmdQueueAdd(const KYGXCmd* cmd);

// Handle queued commands.
void CmdQueueTriggerHandling(void);

#endif /* GUARD_KYGX_CMDQUEUE_H */