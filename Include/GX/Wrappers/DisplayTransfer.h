#ifndef _CTRGX_HELPERS_DISPLAYTRANSFER_H
#define _CTRGX_HELPERS_DISPLAYTRANSFER_H

#include "GX/GX.h"

#define CTRGX_DISPLAYTRANSFER_FMT_RGBA8 0
#define CTRGX_DISPLAYTRANSFER_FMT_RGB8 1
#define CTRGX_DISPLAYTRANSFER_FMT_RGB565 2
#define CTRGX_DISPLAYTRANSFER_FMT_RGB5A1 3
#define CTRGX_DISPLAYTRANSFER_FMT_RGBA4 4

#define CTRGX_DISPLAYTRANSFER_DOWNSCALE_NONE 0
#define CTRGX_DISPLAYTRANSFER_DOWNSCALE_2X1 1
#define CTRGX_DISPLAYTRANSFER_DOWNSCALE_2X2 2

#define CTRGX_DISPLAYTRANSFER_FLAG_VERTICAL_FLIP (1 << 0)
#define CTRGX_DISPLAYTRANSFER_FLAG_MAKE_TILED (1 << 1)
#define CTRGX_DISPLAYTRANSFER_FLAG_DONT_MAKE_LINEAR (5 << 1)
#define CTRGX_DISPLAYTRANSFER_FLAG_SRC_FORMAT(fmt) ((fmt) << 8)
#define CTRGX_DISPLAYTRANSFER_FLAG_DST_FORMAT(fmt) ((fmt) << 12)
#define CTRGX_DISPLAYTRANSFER_FLAG_BLOCKMODE32 (1 << 16)
#define CTRGX_DISPLAYTRANSFER_FLAG_DOWNSCALE(v) ((v) << 24)

typedef struct {
    u8 srcFmt;
    u8 dstFmt;
    u8 downscale;
    bool verticalFlip;
    bool makeTiled;
    bool dontMakeLinear;
    bool blockMode32;
} GXDisplayTransferFlags;

CTRGX_INLINE u32 ctrgxMakeDisplayTransferFlags(const GXDisplayTransferFlags* flags) {
    CTRGX_ASSERT(flags);

    u32 ret = CTRGX_DISPLAYTRANSFER_FLAG_SRC_FORMAT(flags->srcFmt);
    ret |= CTRGX_DISPLAYTRANSFER_FLAG_DST_FORMAT(flags->dstFmt);
    ret |= CTRGX_DISPLAYTRANSFER_FLAG_DOWNSCALE(flags->downscale);

    if (flags->verticalFlip)
        ret |= CTRGX_DISPLAYTRANSFER_FLAG_VERTICAL_FLIP;

    if (flags->makeTiled)
        ret |= CTRGX_DISPLAYTRANSFER_FLAG_MAKE_TILED;

    if (flags->dontMakeLinear)
        ret |= CTRGX_DISPLAYTRANSFER_FLAG_DONT_MAKE_LINEAR;

    if (flags->blockMode32)
        ret |= CTRGX_DISPLAYTRANSFER_FLAG_BLOCKMODE32;

    return ret;
}

CTRGX_INLINE void ctrgxMakeDisplayTransfer(GXCmd* cmd, const void* src, void* dst, u16 srcWidth, u16 srcHeight, u16 dstWidth, u16 dstHeight, u32 flags) {
    CTRGX_ASSERT(cmd);

    // Transfer engine doesn't support anything lower than 64.
    // TODO: dst?
    CTRGX_ASSERT(srcWidth >= 64 && srcHeight >= 64);

    // Bit1 and bit5 are not compatible.
    CTRGX_ASSERT((flags & (CTRGX_DISPLAYTRANSFER_FLAG_MAKE_TILED | CTRGX_DISPLAYTRANSFER_FLAG_DONT_MAKE_LINEAR)) != (CTRGX_DISPLAYTRANSFER_FLAG_MAKE_TILED | CTRGX_DISPLAYTRANSFER_FLAG_DONT_MAKE_LINEAR));

    // Source and destination format must match, or source must be RGBA8 and destination must be RGB8.
    const u8 srcFmt = (flags >> 8) & 0x7;
    const u8 dstFmt = (flags >> 12) & 0x7;
    CTRGX_ASSERT(srcFmt == dstFmt || (srcFmt == CTRGX_DISPLAYTRANSFER_FMT_RGBA8 && dstFmt == CTRGX_DISPLAYTRANSFER_FMT_RGB8));

    cmd->header = CTRGX_CMDID_DISPLAYTRANSFER;

    cmd->params[0] = (u32)src;
    cmd->params[1] = (u32)dst;
    cmd->params[2] = (srcHeight << 16) | srcWidth;
    cmd->params[3] = (dstHeight << 16) | dstWidth;
    cmd->params[4] = flags & ~0x8u;
}

CTRGX_INLINE bool ctrgxAddDisplayTransfer(GXCmdBuffer* b, const void* src, void* dst, u16 srcWidth, u16 srcHeight, u16 dstWidth, u16 dstHeight, u32 flags) {
    CTRGX_ASSERT(b);
    
    GXCmd cmd;
    ctrgxMakeDisplayTransfer(&cmd, src, dst, srcWidth, srcHeight, dstWidth, dstHeight, flags);
    return ctrgxCmdBufferAdd(b, &cmd);    
}

CTRGX_INLINE void ctrgxSyncDisplayTransfer(const void* src, void* dst, u16 srcWidth, u16 srcHeight, u16 dstWidth, u16 dstHeight, u32 flags) {
    GXCmd cmd;
    ctrgxMakeDisplayTransfer(&cmd, src, dst, srcWidth, srcHeight, dstWidth, dstHeight, flags);
    ctrgxExecSync(&cmd);
}

#endif /* _CTRGX_HELPERS_DISPLAYTRANSFER_H */