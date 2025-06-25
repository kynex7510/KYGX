#ifndef _KYGX_GX_H
#define _KYGX_GX_H

#include <GX/Command.h>
#include <GX/Interrupt.h>
#include <GX/CommandBuffer.h>

#define kygxWaitVBlank()         \
    kygxClearIntr(GX_INTR_PDC0); \
    kygxWaitIntr(GX_INTR_PDC0)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

bool kygxInit(void);
void kygxExit(void);
GXCmdBuffer* kygxExchangeCmdBuffer(GXCmdBuffer* b, bool flush);

void kygxLock(void);
bool kygxUnlock(bool exec);
GXIntrQueue* kygxGetIntrQueue(void);
GXCmdQueue* kygxGetCmdQueue(void);
GXCmdBuffer* kygxGetCmdBuffer(void);

void kygxWaitIntr(GXIntr intrID);
void kygxClearIntr(GXIntr intrID);

bool kygxFlushBufferedCommands(void);
void kygxWaitCompletion(void);
void kygxHalt(bool wait);
void kygxExecSync(const GXCmd* cmd);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _KYGX_GX_H */