#ifndef _KYGX_WRAPPERS_DISPLAYTRANSFER_H
#define _KYGX_WRAPPERS_DISPLAYTRANSFER_H

#include <GX/GX.h>

#define KYGX_DISPLAYTRANSFER_MODE_T2L 0
#define KYGX_DISPLAYTRANSFER_MODE_L2T (1 << 1)
#define KYGX_DISPLAYTRANSFER_MODE_T2T (5 << 1)

#define KYGX_DISPLAYTRANSFER_FMT_RGBA8 0
#define KYGX_DISPLAYTRANSFER_FMT_RGB8 1
#define KYGX_DISPLAYTRANSFER_FMT_RGB565 2
#define KYGX_DISPLAYTRANSFER_FMT_RGB5A1 3
#define KYGX_DISPLAYTRANSFER_FMT_RGBA4 4

#define KYGX_DISPLAYTRANSFER_DOWNSCALE_NONE 0
#define KYGX_DISPLAYTRANSFER_DOWNSCALE_2X1 1
#define KYGX_DISPLAYTRANSFER_DOWNSCALE_2X2 2

#define KYGX_DISPLAYTRANSFER_FLAG_VERTICAL_FLIP (1 << 0)
#define KYGX_DISPLAYTRANSFER_FLAG_SRC_FORMAT(fmt) ((fmt) << 8)
#define KYGX_DISPLAYTRANSFER_FLAG_DST_FORMAT(fmt) ((fmt) << 12)
#define KYGX_DISPLAYTRANSFER_FLAG_BLOCKMODE32 (1 << 16)
#define KYGX_DISPLAYTRANSFER_FLAG_DOWNSCALE(v) ((v) << 24)

typedef struct {
    u8 mode;
    u8 srcFmt;
    u8 dstFmt;
    u8 downscale;
    bool verticalFlip;
    bool blockMode32;
} GXDisplayTransferFlags;

KYGX_INLINE u32 kygxMakeDisplayTransferFlags(const GXDisplayTransferFlags* flags) {
    KYGX_ASSERT(flags);

    KYGX_ASSERT(flags->mode == KYGX_DISPLAYTRANSFER_MODE_T2L ||
        flags->mode == KYGX_DISPLAYTRANSFER_MODE_L2T ||
        flags->mode == KYGX_DISPLAYTRANSFER_MODE_T2T);
    u32 ret = flags->mode;

    KYGX_ASSERT(flags->srcFmt <= KYGX_DISPLAYTRANSFER_FMT_RGBA4);
    ret |= KYGX_DISPLAYTRANSFER_FLAG_SRC_FORMAT(flags->srcFmt);

    KYGX_ASSERT(flags->dstFmt <= KYGX_DISPLAYTRANSFER_FMT_RGBA4);
    ret |= KYGX_DISPLAYTRANSFER_FLAG_DST_FORMAT(flags->dstFmt);

    KYGX_ASSERT(flags->downscale <= KYGX_DISPLAYTRANSFER_DOWNSCALE_2X2);
    ret |= KYGX_DISPLAYTRANSFER_FLAG_DOWNSCALE(flags->downscale);

    if (flags->verticalFlip)
        ret |= KYGX_DISPLAYTRANSFER_FLAG_VERTICAL_FLIP;

    if (flags->blockMode32)
        ret |= KYGX_DISPLAYTRANSFER_FLAG_BLOCKMODE32;

    return ret;
}

KYGX_INLINE void kygxMakeDisplayTransfer(GXCmd* cmd, const void* src, void* dst, u16 srcWidth, u16 srcHeight, u16 dstWidth, u16 dstHeight, u32 flags) {
    KYGX_ASSERT(cmd);

    // TODO: bit1 and bit5?
    //KYGX_ASSERT((flags & (KYGX_DISPLAYTRANSFER_FLAG_MAKE_TILED | KYGX_DISPLAYTRANSFER_FLAG_DONT_MAKE_LINEAR)) != (KYGX_DISPLAYTRANSFER_FLAG_MAKE_TILED | KYGX_DISPLAYTRANSFER_FLAG_DONT_MAKE_LINEAR));

    // Transfer engine doesn't support anything lower than 64.
    // TODO: dst?
    KYGX_ASSERT(srcWidth >= 64 && srcHeight >= 64);

    // Source and destination format must match, or source must be RGBA8 and destination must be RGB8.
    const u8 srcFmt = (flags >> 8) & 0x7;
    const u8 dstFmt = (flags >> 12) & 0x7;
    KYGX_ASSERT(srcFmt == dstFmt || (srcFmt == KYGX_DISPLAYTRANSFER_FMT_RGBA8 && dstFmt == KYGX_DISPLAYTRANSFER_FMT_RGB8));

    cmd->header = KYGX_CMDID_DISPLAYTRANSFER;

    cmd->params[0] = (u32)src;
    cmd->params[1] = (u32)dst;
    cmd->params[2] = (srcHeight << 16) | srcWidth;
    cmd->params[3] = (dstHeight << 16) | dstWidth;
    cmd->params[4] = flags & ~0x8u;
}

KYGX_INLINE bool kygxAddDisplayTransfer(GXCmdBuffer* b, const void* src, void* dst, u16 srcWidth, u16 srcHeight, u16 dstWidth, u16 dstHeight, u32 flags) {
    KYGX_ASSERT(b);
    
    GXCmd cmd;
    kygxMakeDisplayTransfer(&cmd, src, dst, srcWidth, srcHeight, dstWidth, dstHeight, flags);
    return kygxCmdBufferAdd(b, &cmd);    
}

KYGX_INLINE void kygxSyncDisplayTransfer(const void* src, void* dst, u16 srcWidth, u16 srcHeight, u16 dstWidth, u16 dstHeight, u32 flags) {
    GXCmd cmd;
    kygxMakeDisplayTransfer(&cmd, src, dst, srcWidth, srcHeight, dstWidth, dstHeight, flags);
    kygxExecSync(&cmd);
}

#endif /* _KYGX_WRAPPERS_DISPLAYTRANSFER_H */