#ifndef _CTRGX_WRAPPERS_REQUESTDMA_H
#define _CTRGX_WRAPPERS_REQUESTDMA_H

// Baremetal doesn't support this command.
#ifndef CTRGX_BAREMETAL

#include "GX/GX.h"

CTRGX_INLINE void ctrgxMakeRequestDMA(GXCmd* cmd, const void* src, void* dst, size_t size, bool flush) {
    CTRGX_ASSERT(cmd);

    cmd->header = CTRGX_CMDID_REQUESTDMA;
    cmd->params[0] = (u32)src;
    cmd->params[1] = (u32)dst;
    cmd->params[2] = size;
    cmd->params[3] = cmd->params[4] = cmd->params[5] = 0;
    cmd->params[6] = flush ? 1 : 0;
}

CTRGX_INLINE bool ctrgxAddRequestDMA(GXCmdBuffer* b, const void* src, void* dst, size_t size, bool flush) {
    CTRGX_ASSERT(b);

    GXCmd cmd;
    ctrgxMakeRequestDMA(&cmd, src, dst, size, flush);
    return ctrgxCmdBufferAdd(b, &cmd);
}

CTRGX_INLINE void ctrgxSyncRequestDMA(const void* src, void* dst, size_t size, bool flush) {
    GXCmd cmd;
    ctrgxMakeRequestDMA(&cmd, src, dst, size, flush);
    ctrgxExecSync(&cmd);
}

#endif // !CTRGX_BAREMETAL

#endif /* _CTRGX_WRAPPERS_REQUESTDMA_H */