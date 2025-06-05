#ifndef _KYGX_WRAPPERS_TEXTURECOPY_H
#define _KYGX_WRAPPERS_TEXTURECOPY_H

#include <GX/GX.h>

#define KYGX_TEXTURECOPY_PIXEL_SIZE_RGBA8 4
#define KYGX_TEXTURECOPY_PIXEL_SIZE_RGB8 3
#define KYGX_TEXTURECOPY_PIXEL_SIZE_RGB565 2
#define KYGX_TEXTURECOPY_PIXEL_SIZE_RGB5A1 2
#define KYGX_TEXTURECOPY_PIXEL_SIZE_RGBA4 2

typedef struct {
    u16 x;
    u16 y;
    u16 width;
    u16 height;
} GXTextureCopyRect;

KYGX_INLINE void kygxConvertTextureCopyRect(const GXTextureCopyRect* rect, u16 surfaceWidth, u8 pixelSize, size_t* offset, size_t* size, u16* lineWidth, u16* gap) {
    KYGX_ASSERT(rect);

    if (offset)
        *offset = (surfaceWidth * rect->y * pixelSize) + (rect->x * pixelSize);

    if (size)
        *size = rect->width * rect->height * pixelSize;

    if (lineWidth)
        *lineWidth = (rect->width * pixelSize) >> 4;

    if (gap)
        *gap = ((surfaceWidth - rect->width) * pixelSize) >> 4;
}

KYGX_INLINE void kygxConvertTextureCopyRectRotated(const GXTextureCopyRect* rect, u16 surfaceHeight, u8 pixelSize, size_t* offset, size_t* size, u16* lineWidth, u16* gap) {
    KYGX_ASSERT(rect);

    GXTextureCopyRect tmp;
    tmp.x = rect->y;
    tmp.y = rect->x;
    tmp.width = rect->height;
    tmp.height = rect->width;
    kygxConvertTextureCopyRect(&tmp, surfaceHeight, pixelSize, offset, size, lineWidth, gap);
}

KYGX_INLINE void kygxMakeTextureCopy(GXCmd* cmd, const void* src, void* dst, size_t size, u16 srcLineWidth, u16 srcGap, u16 dstLineWidth, u16 dstGap) {
    KYGX_ASSERT(cmd);

    u32 flags = 0;
    if (srcGap || dstGap)
        flags = 0x4;

    // TODO: dst width? dst gap?
    if (srcGap) {
        KYGX_ASSERT(size >= 192);
        KYGX_ASSERT(srcLineWidth);
    } else {
        KYGX_ASSERT(size >= 16);
    }

    cmd->header = KYGX_CMDID_TEXTURECOPY;

    cmd->params[0] = (u32)src;
    cmd->params[1] = (u32)dst;
    cmd->params[2] = size;
    cmd->params[3] = (srcGap << 16) | srcLineWidth;
    cmd->params[4] = (dstGap << 16) | dstLineWidth;
    cmd->params[5] = flags | 0x8;
    cmd->params[6] = 0;
}

KYGX_INLINE bool kygxAddTextureCopy(GXCmdBuffer* b, const void* src, void* dst, size_t size, u16 srcLineWidth, u16 srcGap, u16 dstLineWidth, u16 dstGap) {
    KYGX_ASSERT(b);
    
    GXCmd cmd;
    kygxMakeTextureCopy(&cmd, src, dst, size, srcLineWidth, srcGap, dstLineWidth, dstGap);
    return kygxCmdBufferAdd(b, &cmd);    
}

KYGX_INLINE void kygxSyncTextureCopy(const void* src, void* dst, size_t size, u16 srcLineWidth, u16 srcGap, u16 dstLineWidth, u16 dstGap) {
    GXCmd cmd;
    kygxMakeTextureCopy(&cmd, src, dst, size, srcLineWidth, srcGap, dstLineWidth, dstGap);
    kygxExecSync(&cmd);
}

#endif /* _KYGX_WRAPPERS_TEXTURECOPY_H */