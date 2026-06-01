/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_KYGX_GXSERVER_H
#define GUARD_KYGX_GXSERVER_H

#include <KYGX/GX.h>

#include "BatchQueue.h"

typedef void (*GXOnInterrupt)(KYGXIntr intrID);
typedef void (*GXOnBatchCompleted)(void);

typedef enum {
    GXExecState_Success,
    GXExecState_NoCommands,
    GXExecState_NoMemory,
    GXExecState_Busy,
} GXExecState;

void GXServerInit(void);
void GXServerExit(void);
void GXServerSetCallbacks(GXOnInterrupt onInterrupt, GXOnBatchCompleted onBatchCompleted);
GXExecState GXServerExec(CmdIterator* it);

#endif /* GUARD_KYGX_GXSERVER_H */