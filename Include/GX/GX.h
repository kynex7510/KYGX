#ifndef _KYGX_GX_H
#define _KYGX_GX_H

#include <GX/Command.h>
#include <GX/Interrupt.h>
#include <GX/CommandBuffer.h>

#define kygxWaitVBlank()         \
    kygxClearIntr(GX_INTR_PDC0); \
    kygxWaitIntr(GX_INTR_PDC0)

KYGX_EXTERN bool kygxInit(void);
KYGX_EXTERN void kygxExit(void);
KYGX_EXTERN GXCmdBuffer* kygxExchangeCmdBuffer(GXCmdBuffer* b, bool flush);

KYGX_EXTERN void kygxLock(void);
KYGX_EXTERN bool kygxUnlock(bool exec);
KYGX_EXTERN GXIntrQueue* kygxGetIntrQueue(void);
KYGX_EXTERN GXCmdQueue* kygxGetCmdQueue(void);
KYGX_EXTERN GXCmdBuffer* kygxGetCmdBuffer(void);

KYGX_EXTERN void kygxWaitIntr(GXIntr intrID);
KYGX_EXTERN void kygxClearIntr(GXIntr intrID);

KYGX_EXTERN bool kygxFlushBufferedCommands(void);
KYGX_EXTERN void kygxWaitCompletion(void);
KYGX_EXTERN void kygxHalt(bool wait);
KYGX_EXTERN void kygxExecSync(const GXCmd* cmd);

#endif /* _KYGX_GX_H */