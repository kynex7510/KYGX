/**
 * @file TextureCopy.h
 * @brief Implementation of the TextureCopy command.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_KYGX_WRAPPERS_TEXTURECOPY_H
#define GUARD_KYGX_WRAPPERS_TEXTURECOPY_H

#include <CTR/Assert.h>

#include <KYGX/GX.h>

#define KYGX_TEXTURECOPY_PIXEL_SIZE_RGBA8 4
#define KYGX_TEXTURECOPY_PIXEL_SIZE_RGB8 3
#define KYGX_TEXTURECOPY_PIXEL_SIZE_RGB565 2
#define KYGX_TEXTURECOPY_PIXEL_SIZE_RGB5A1 2
#define KYGX_TEXTURECOPY_PIXEL_SIZE_RGBA4 2

typedef struct {
    void* addr;
    uint16_t width;
    uint16_t height;
    uint8_t pixelSize;
    bool rotated; // The surface is rotated 90 degrees CCW.
} KYGXTextureCopySurface;

typedef struct {
    // Normal: (0, 0) is top left.
    // Rotated: (0, 0) is bottom left.
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} KYGXTextureCopyRect;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

CTR_INLINE void kygxGetTextureCopyRectParams(const KYGXTextureCopySurface* surface, const KYGXTextureCopyRect* rect, size_t* offset, size_t* size, uint16_t* lineWidth, uint16_t* gap) {
    CTR_ASSERT(surface);
    CTR_ASSERT(rect);
    CTR_ASSERT(surface->width >= rect->width);
    CTR_ASSERT(surface->height >= rect->height);

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

CTR_INLINE void kygxMakeTextureCopy(KYGXCmd* cmd, const void* src, void* dst, size_t size, uint16_t srcLineWidth, uint16_t srcGap, uint16_t dstLineWidth, uint16_t dstGap) {
    CTR_ASSERT(cmd);

    uint32_t flags = 0;
    if (srcGap || dstGap) {
        CTR_ASSERT(size >= 192);

        if (srcGap) {
            CTR_ASSERT(srcLineWidth);
        }

        if (dstGap) {
            CTR_ASSERT(dstLineWidth);
        }

        flags = 0x4;
    } else {
        CTR_ASSERT(size >= 16);
    }

    cmd->header = KYGX_CMD_TEXTURECOPY;

    cmd->params[0] = (uint32_t)src;
    cmd->params[1] = (uint32_t)dst;
    cmd->params[2] = size;
    cmd->params[3] = (srcGap << 16) | srcLineWidth;
    cmd->params[4] = (dstGap << 16) | dstLineWidth;
    cmd->params[5] = flags | 0x8; // Enforce TextureCopy bit.
    cmd->params[6] = 0;
}

CTR_INLINE void kygxMakeRectCopy(KYGXCmd* cmd, const KYGXTextureCopySurface* srcSurface, const KYGXTextureCopyRect* srcRect, const KYGXTextureCopySurface* dstSurface, const KYGXTextureCopyRect* dstRect) {
    CTR_ASSERT(cmd);
    CTR_ASSERT(srcSurface);
    CTR_ASSERT(srcRect);
    CTR_ASSERT(dstSurface);
    CTR_ASSERT(dstRect);

    size_t srcOffset = 0;
    size_t srcSize = 0;
    uint16_t srcLineWidth = 0;
    uint16_t srcGap = 0;
    kygxGetTextureCopyRectParams(srcSurface, srcRect, &srcOffset, &srcSize, &srcLineWidth, &srcGap);

    size_t dstOffset = 0;
    size_t dstSize = 0;
    uint16_t dstLineWidth = 0;
    uint16_t dstGap = 0;
    kygxGetTextureCopyRectParams(dstSurface, dstRect, &dstOffset, &dstSize, &dstLineWidth, &dstGap);

    CTR_ASSERT(srcSize == dstSize);

    kygxMakeTextureCopy(cmd, (const uint8_t*)srcSurface->addr + srcOffset, (uint8_t*)dstSurface->addr + dstOffset, srcSize, srcLineWidth, srcGap, dstLineWidth, dstGap);
}

CTR_INLINE void kygxSyncTextureCopy(const void* src, void* dst, size_t size, uint16_t srcLineWidth, uint16_t srcGap, uint16_t dstLineWidth, uint16_t dstGap) {
    KYGXCmd cmd;
    kygxMakeTextureCopy(&cmd, src, dst, size, srcLineWidth, srcGap, dstLineWidth, dstGap);
    kygxExecSync(&cmd);
}

CTR_INLINE void kygxSyncRectCopy(const KYGXTextureCopySurface* srcSurface, const KYGXTextureCopyRect* srcRect, const KYGXTextureCopySurface* dstSurface, const KYGXTextureCopyRect* dstRect) {
    CTR_ASSERT(srcSurface);
    CTR_ASSERT(srcRect);
    CTR_ASSERT(dstSurface);
    CTR_ASSERT(dstRect);
    
    KYGXCmd cmd;
    kygxMakeRectCopy(&cmd, srcSurface, srcRect, dstSurface, dstRect);
    kygxExecSync(&cmd);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* GUARD_KYGX_WRAPPERS_TEXTURECOPY_H */