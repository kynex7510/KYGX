#ifndef _KYGX_WRAPPERS_TEXTURECOPY_H
#define _KYGX_WRAPPERS_TEXTURECOPY_H

#include <KYGX/GX.h>

#define KYGX_TEXTURECOPY_PIXEL_SIZE_RGBA8 4
#define KYGX_TEXTURECOPY_PIXEL_SIZE_RGB8 3
#define KYGX_TEXTURECOPY_PIXEL_SIZE_RGB565 2
#define KYGX_TEXTURECOPY_PIXEL_SIZE_RGB5A1 2
#define KYGX_TEXTURECOPY_PIXEL_SIZE_RGBA4 2

typedef struct {
    void* addr;
    u16 width;
    u16 height;
    u8 pixelSize;
    bool rotated; // The surface is rotated 90 degrees CCW.
} KYGXTextureCopySurface;

typedef struct {
    // Normal: (0, 0) is top left.
    // Rotated: (0, 0) is bottom left.
    u16 x;
    u16 y;
    u16 width;
    u16 height;
} KYGXTextureCopyRect;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

KYGX_INLINE void kygxGetTextureCopyRectParams(const KYGXTextureCopySurface* surface, const KYGXTextureCopyRect* rect, size_t* offset, size_t* size, u16* lineWidth, u16* gap) {
    KYGX_ASSERT(surface);
    KYGX_ASSERT(rect);
    KYGX_ASSERT(surface->width >= rect->width);
    KYGX_ASSERT(surface->height >= rect->height);

    size_t surfaceWidth = 0;
    size_t rectWidth = 0;
    size_t xpos = 0;
    size_t ypos = 0;

    if (surface->rotated) {
        surfaceWidth = surface->height;
        rectWidth = rect->height;
        xpos = rect->y;
        ypos = rect->x;
    } else {
        surfaceWidth = surface->width;
        rectWidth = rect->width;
        xpos = rect->x;
        ypos = rect->y;
    }

    if (offset)
        *offset = (surfaceWidth * ypos * surface->pixelSize) + (xpos * surface->pixelSize);

    if (size)
        *size = rect->width * rect->height * surface->pixelSize;

    if (lineWidth)
        *lineWidth = (rectWidth * surface->pixelSize) >> 4;

    if (gap)
        *gap = ((surfaceWidth - rectWidth) * surface->pixelSize) >> 4;
}

KYGX_INLINE void kygxMakeTextureCopy(KYGXCmd* cmd, const void* src, void* dst, size_t size, u16 srcLineWidth, u16 srcGap, u16 dstLineWidth, u16 dstGap) {
    KYGX_ASSERT(cmd);

    u32 flags = 0;
    if (srcGap || dstGap) {
        KYGX_ASSERT(size >= 192);

        if (srcGap) {
            KYGX_ASSERT(srcLineWidth);
        }

        if (dstGap) {
            KYGX_ASSERT(dstLineWidth);
        }

        flags = 0x4;
    } else {
        KYGX_ASSERT(size >= 16);
    }

    cmd->header = KYGX_CMDID_TEXTURECOPY;

    cmd->params[0] = (u32)src;
    cmd->params[1] = (u32)dst;
    cmd->params[2] = size;
    cmd->params[3] = (srcGap << 16) | srcLineWidth;
    cmd->params[4] = (dstGap << 16) | dstLineWidth;
    cmd->params[5] = flags | 0x8; // Enforce TextureCopy bit.
    cmd->params[6] = 0;
}

KYGX_INLINE void kygxMakeRectCopy(KYGXCmd* cmd, const KYGXTextureCopySurface* srcSurface, const KYGXTextureCopyRect* srcRect, const KYGXTextureCopySurface* dstSurface, const KYGXTextureCopyRect* dstRect) {
    KYGX_ASSERT(cmd);
    KYGX_ASSERT(srcSurface);
    KYGX_ASSERT(srcRect);
    KYGX_ASSERT(dstSurface);
    KYGX_ASSERT(dstRect);

    size_t srcOffset = 0;
    size_t srcSize = 0;
    u16 srcLineWidth = 0;
    u16 srcGap = 0;
    kygxGetTextureCopyRectParams(srcSurface, srcRect, &srcOffset, &srcSize, &srcLineWidth, &srcGap);

    size_t dstOffset = 0;
    size_t dstSize = 0;
    u16 dstLineWidth = 0;
    u16 dstGap = 0;
    kygxGetTextureCopyRectParams(dstSurface, dstRect, &dstOffset, &dstSize, &dstLineWidth, &dstGap);

    KYGX_ASSERT(srcSize == dstSize);

    kygxMakeTextureCopy(cmd, (const u8*)srcSurface->addr + srcOffset, (u8*)dstSurface->addr + dstOffset, srcSize, srcLineWidth, srcGap, dstLineWidth, dstGap);
}

KYGX_INLINE bool kygxAddTextureCopy(KYGXCmdBuffer* b, const void* src, void* dst, size_t size, u16 srcLineWidth, u16 srcGap, u16 dstLineWidth, u16 dstGap) {
    KYGX_ASSERT(b);
    
    KYGXCmd cmd;
    kygxMakeTextureCopy(&cmd, src, dst, size, srcLineWidth, srcGap, dstLineWidth, dstGap);
    return kygxCmdBufferAdd(b, &cmd);    
}

KYGX_INLINE bool kygxAddRectCopy(KYGXCmdBuffer* b, const KYGXTextureCopySurface* srcSurface, const KYGXTextureCopyRect* srcRect, const KYGXTextureCopySurface* dstSurface, const KYGXTextureCopyRect* dstRect) {
    KYGX_ASSERT(b);
    KYGX_ASSERT(srcSurface);
    KYGX_ASSERT(srcRect);
    KYGX_ASSERT(dstSurface);
    KYGX_ASSERT(dstRect);
    
    KYGXCmd cmd;
    kygxMakeRectCopy(&cmd, srcSurface, srcRect, dstSurface, dstRect);
    return kygxCmdBufferAdd(b, &cmd);
}

KYGX_INLINE void kygxSyncTextureCopy(const void* src, void* dst, size_t size, u16 srcLineWidth, u16 srcGap, u16 dstLineWidth, u16 dstGap) {
    KYGXCmd cmd;
    kygxMakeTextureCopy(&cmd, src, dst, size, srcLineWidth, srcGap, dstLineWidth, dstGap);
    kygxExecSync(&cmd);
}

KYGX_INLINE void kygxSyncRectCopy(const KYGXTextureCopySurface* srcSurface, const KYGXTextureCopyRect* srcRect, const KYGXTextureCopySurface* dstSurface, const KYGXTextureCopyRect* dstRect) {
    KYGX_ASSERT(srcSurface);
    KYGX_ASSERT(srcRect);
    KYGX_ASSERT(dstSurface);
    KYGX_ASSERT(dstRect);
    
    KYGXCmd cmd;
    kygxMakeRectCopy(&cmd, srcSurface, srcRect, dstSurface, dstRect);
    kygxExecSync(&cmd);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _KYGX_WRAPPERS_TEXTURECOPY_H */