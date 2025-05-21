#ifndef _CTRGX_GX_H
#define _CTRGX_GX_H

#include "GX/Command.h"
#include "GX/Interrupt.h"
#include "GX/CommandBuffer.h"

CTRGX_EXTERN bool ctrgxInit(void);
CTRGX_EXTERN void ctrgxExit(void);
CTRGX_EXTERN GXCmdBuffer* ctrgxExchangeCmdBuffer(GXCmdBuffer* b, bool flush);

CTRGX_EXTERN void ctrgxLock(void);
CTRGX_EXTERN void ctrgxUnlock(bool exec);
CTRGX_EXTERN GXIntrQueue* ctrgxGetIntrQueue(void);
CTRGX_EXTERN GXCmdQueue* ctrgxGetCmdQueue(void);
CTRGX_EXTERN GXCmdBuffer* ctrgxGetCmdBuffer(void);

CTRGX_EXTERN GXIntr ctrgxWaitAnyIntr(void);
CTRGX_EXTERN void ctrgxWaitIntr(GXIntr intrID);
CTRGX_EXTERN void ctrgxClearIntr(GXIntr intrID);

CTRGX_EXTERN bool ctrgxFlushBufferedCommands(void);
CTRGX_EXTERN void ctrgxWaitCompletion(void);
CTRGX_EXTERN void ctrgxHalt(bool wait);
CTRGX_EXTERN void ctrgxExecSync(const GXCmd* cmd);

#endif /* _CTRGX_GX_H */