#ifndef _CTRGX_HELPERS_TEXTURECOPY_H
#define _CTRGX_HELPERS_TEXTURECOPY_H

#include "GX/GX.h"

#define CTRGX_TEXTURECOPY_PIXEL_SIZE_RGBA8 4
#define CTRGX_TEXTURECOPY_PIXEL_SIZE_RGB8 3
#define CTRGX_TEXTURECOPY_PIXEL_SIZE_RGB565 2
#define CTRGX_TEXTURECOPY_PIXEL_SIZE_RGB5A1 2
#define CTRGX_TEXTURECOPY_PIXEL_SIZE_RGBA4 2

typedef struct {
    u16 x;
    u16 y;
    u16 width;
    u16 height;
} GXTextureCopyRect;

CTRGX_INLINE void ctrgxConvertTextureCopyRect(const GXTextureCopyRect* rect, u16 screenWidth, u8 pixelSize, size_t* offset, size_t* size, u16* lineWidth, u16* gap) {
    CTRGX_ASSERT(rect);

    if (offset)
        *offset = (screenWidth * rect->y * pixelSize) + (rect->x * pixelSize);

    if (size)
        *size = rect->width * rect->height * pixelSize;

    if (lineWidth)
        *lineWidth = (rect->width * pixelSize) >> 4;

    if (gap)
        *gap = ((screenWidth - rect->width) * pixelSize) >> 4;
}

CTRGX_INLINE void ctrgxConvertTextureCopyRectRotated(const GXTextureCopyRect* rect, u16 screenHeight, u8 pixelSize, size_t* offset, size_t* size, u16* lineWidth, u16* gap) {
    CTRGX_ASSERT(rect);

    GXTextureCopyRect tmp;
    tmp.x = rect->y;
    tmp.y = rect->x;
    tmp.width = rect->height;
    tmp.height = rect->width;
    ctrgxConvertTextureCopyRect(&tmp, screenHeight, pixelSize, offset, size, lineWidth, gap);
}

CTRGX_INLINE void ctrgxMakeTextureCopy(GXCmd* cmd, const void* src, void* dst, size_t size, u16 srcLineWidth, u16 srcGap, u16 dstLineWidth, u16 dstGap) {
    CTRGX_ASSERT(cmd);

    u32 flags = 0;
    if (srcGap || dstGap)
        flags = 0x4;

    // TODO: dst width? dst gap?
    if (srcGap) {
        CTRGX_ASSERT(size >= 192);
        CTRGX_ASSERT(srcLineWidth);
    } else {
        CTRGX_ASSERT(size >= 16);
    }

    cmd->header = CTRGX_CMDID_TEXTURECOPY;

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