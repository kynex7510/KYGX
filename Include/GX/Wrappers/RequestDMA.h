#ifndef _KYGX_WRAPPERS_REQUESTDMA_H
#define _KYGX_WRAPPERS_REQUESTDMA_H

// Baremetal doesn't support this command.
#ifndef KYGX_BAREMETAL

#include <GX/GX.h>

KYGX_INLINE void kygxMakeRequestDMA(GXCmd* cmd, const void* src, void* dst, size_t size, bool flush) {
    KYGX_ASSERT(cmd);

    cmd->header = KYGX_CMDID_REQUESTDMA;
    cmd->params[0] = (u32)src;
    cmd->params[1] = (u32)dst;
    cmd->params[2] = size;
    cmd->params[3] = cmd->params[4] = cmd->params[5] = 0;
    cmd->params[6] = flush ? 1 : 0;
}

KYGX_INLINE bool kygxAddRequestDMA(GXCmdBuffer* b, const void* src, void* dst, size_t size, bool flush) {
    KYGX_ASSERT(b);

    GXCmd cmd;
    kygxMakeRequestDMA(&cmd, src, dst, size, flush);
    return kygxCmdBufferAdd(b, &cmd);
}

KYGX_INLINE void kygxSyncRequestDMA(const void* src, void* dst, size_t size, bool flush) {
    GXCmd cmd;
    kygxMakeRequestDMA(&cmd, src, dst, size, flush);
    kygxExecSync(&cmd);
}

#endif // !KYGX_BAREMETAL

#endif /* _KYGX_WRAPPERS_REQUESTDMA_H */