/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _KYGX_GX_H
#define _KYGX_GX_H

#include <KYGX/Command.h>
#include <KYGX/Interrupt.h>
#include <KYGX/CommandBuffer.h>

#define kygxWaitVBlank()           \
    kygxClearIntr(KYGX_INTR_PDC0); \
    kygxWaitIntr(KYGX_INTR_PDC0)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

bool kygxInit(void);
void kygxExit(void);
KYGXCmdBuffer* kygxExchangeCmdBuffer(KYGXCmdBuffer* b, bool flush);

void kygxLock(void);
bool kygxUnlock(bool exec);
KYGXIntrQueue* kygxGetIntrQueue(void);
KYGXCmdQueue* kygxGetCmdQueue(void);
KYGXCmdBuffer* kygxGetCmdBuffer(void);

void kygxWaitIntr(KYGXIntr intrID);
void kygxClearIntr(KYGXIntr intrID);

bool kygxFlushBufferedCommands(void);
void kygxWaitCompletion(void);
void kygxHalt(bool wait);
void kygxExecSync(const KYGXCmd* cmd);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _KYGX_GX_H */