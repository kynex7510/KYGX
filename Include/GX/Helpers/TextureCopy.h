#ifndef _CTRGX_HELPERS_TEXTURECOPY_H
#define _CTRGX_HELPERS_TEXTURECOPY_H

#include "GX/GX.h"

CTRGX_INLINE void ctrgxMakeTextureCopy(GXCmd* cmd, const void* src, void* dst, size_t size, u16 srcLineWidth, u16 srcGap, u16 dstLineWidth, u16 dstGap) {
    CTRGX_ASSERT(cmd);

    // TODO: dst width? dst gap?

    u32 flags = 0;
    if (srcGap) {
        CTRGX_ASSERT(size >= 192);
        CTRGX_ASSERT(srcLineWidth);
        flags |= 0x4;
    } else {
        CTRGX_ASSERT(size >= 16);
    }

    cmd->header = CTRGX_CMDID_TEXTURE_COPY;

    cmd->params[0] = (u32)src;
    cmd->params[1] = (u32)dst;
    cmd->params[2] = size;
    cmd->params[3] = (srcGap << 16) | srcLineWidth;
    cmd->params[4] = (dstGap << 16) | dstLineWidth;
    cmd->params[5] = flags | 0x8;
    cmd->params[6] = 0;
}

CTRGX_INLINE bool ctrgxAddTextureCopy(GXCmdBuffer* b, const void* src, void* dst, size_t size, u16 srcLineWidth, u16 srcGap, u16 dstLineWidth, u16 dstGap) {
    CTRGX_ASSERT(b);
    
    GXCmd cmd;
    ctrgxMakeTextureCopy(&cmd, src, dst, size, srcLineWidth, srcGap, dstLineWidth, dstGap);
    return ctrgxCmdBufferAdd(b, &cmd);    
}

CTRGX_INLINE void ctrgxSyncTextureCopy(const void* src, void* dst, size_t size, u16 srcLineWidth, u16 srcGap, u16 dstLineWidth, u16 dstGap) {
    GXCmd cmd;
    ctrgxMakeTextureCopy(&cmd, src, dst, size, srcLineWidth, srcGap, dstLineWidth, dstGap);
    ctrgxExecSync(&cmd);
}

#endif /* _CTRGX_HELPERS_TEXTURECOPY_H */