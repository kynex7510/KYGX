/**
 * @file TextureCopy.h
 * @brief Implementation of the TextureCopy command.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_KYGX_COMMAND_TEXTURECOPY_H
#define GUARD_KYGX_COMMAND_TEXTURECOPY_H

#include <CTR11/Assert.h>

#include <KYGX/GX.h>

/// @brief Common pixel sizes.
typedef enum {
    KYGXPixelSize_RGBA8 = 4,  ///< 4 bytes.
    KYGXPixelSize_RGB8 = 3,   ///< 3 bytes.
    KYGXPixelSize_RGB565 = 2, ///< 2 bytes.
    KYGXPixelSize_RGB5A1 = 2, ///< 2 bytes.
    KYGXPixelSize_RGBA4 = 2,  ///< 2 bytes.
} KYGXPixelSize;

/// @brief Represents a pixel buffer in a copy operation.
typedef struct {
    void* addr;       ///< Buffer address.
    uint16_t width;   ///< Buffer width.
    uint16_t height;  ///< Buffer height.
    size_t pixelSize; ///< Pixel size.
} KYGXSurface;

/// @brief Represents a sub-rectangle of a surface.
typedef struct {
    uint16_t x;      ///< Rectangle X, relative to surface X.
    uint16_t y;      ///< Rectangle Y, relative to surface Y.
    uint16_t width;  ///< Rectangle width.
    uint16_t height; ///< Rectangle height.
} KYGXRect;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief Construct parameters for a RectCopy.
 * @param[in] surface Surface.
 * @param[in] rect Rect.
 * @param[out] offset Buffer offset.
 * @param[out] size Buffer size.
 * @param[out] lineWidth Line width.
 * @param[out] gap Gap.
 */
CTR_INLINE void kygxGetRectCopyParams(const KYGXSurface* surface, const KYGXRect* rect, size_t* offset, size_t* size, uint16_t* lineWidth, uint16_t* gap) {
    CTR_ASSERT(surface);
    CTR_ASSERT(rect);
    CTR_ASSERT(surface->width >= rect->width);
    CTR_ASSERT(surface->height >= rect->height);

    if (offset)
        *offset = (surface->width * rect->y * surface->pixelSize) + (rect->x * surface->pixelSize);

    if (size)
        *size = rect->width * rect->height * surface->pixelSize;

    if (lineWidth)
        *lineWidth = (rect->width * surface->pixelSize) >> 4;

    if (gap)
        *gap = ((surface->width - rect->width) * surface->pixelSize) >> 4;
}

/**
 * @brief Construct a TextureCopy command.
 * This command allows to copy memory using the GPU. Both source and destination addresses must be in FCRAM, VRAM
 * or QTMRAM, and must be aligned to 8 bytes. It's also possible to set a gap between lines, which allows to copy
 * arbitrary sub-rectangles between differently sized surfaces.
 * @param[out] cmd Output command.
 * @param[in] src Source buffer.
 * @param[in] dst Destination buffer.
 * @param[in] size Size.
 * @param[in] srcLineWidth Source line width.
 * @param[in] srcGap Source gap.
 * @param[in] dstLineWidth Destination line width.
 * @param[in] dstGap Destination gap.
 */
CTR_INLINE void kygxMakeTextureCopy(KYGXCmd* cmd, const void* src, void* dst, size_t size, uint16_t srcLineWidth, uint16_t srcGap, uint16_t dstLineWidth, uint16_t dstGap) {
    CTR_ASSERT(cmd);
    CTR_ASSERT(src);
    CTR_ASSERT(dst);
    CTR_ASSERT(IsAligned((uint32_t)src, 8));
    CTR_ASSERT(IsAligned((uint32_t)dst, 8));
    CTR_ASSERT(IsAccessible(src, size, MemAccess_GPURead));
    CTR_ASSERT(IsAccessible(dst, size, MemAccess_GPUWrite));

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

/**
 * @brief Construct a TextureCopy command.
 * This wrapper can be used to copy raw buffers.
 * @param[out] cmd Output command.
 * @param[in] src Source buffer.
 * @param[in] dst Destination buffer.
 * @param[in] size Buffer size.
 */
CTR_INLINE void kygxMakeMemoryCopy(KYGXCmd* cmd, const void* src, void* dst, size_t size) {
    CTR_ASSERT(cmd);
    CTR_ASSERT(src);
    CTR_ASSERT(dst);
    kygxMakeTextureCopy(cmd, src, dst, size, 0, 0, 0, 0);
}

/**
 * @brief Construct a TextureCopy command.
 * This wrapper can be used to copy surfaces.
 * @param[out] cmd Output command.
 * @param[in] src Source surface.
 * @param[in] dst Destination surface.
 */
CTR_INLINE void kygxMakeSurfaceCopy(KYGXCmd* cmd, const KYGXSurface* src, const KYGXSurface* dst) {
    CTR_ASSERT(cmd);
    CTR_ASSERT(src);
    CTR_ASSERT(dst);

    const size_t srcSize = src->width * src->height * src->pixelSize;
    const size_t dstSize = dst->width * dst->height * dst->pixelSize;
    CTR_ASSERT(srcSize == dstSize);

    kygxMakeTextureCopy(cmd, src->addr, dst->addr, srcSize, 0, 0, 0, 0);
}

/**
 * @brief Construct a TextureCopy command.
 * This wrapper can be used to copy rects between surfaces.
 * @param[out] cmd Output command.
 * @param[in] src Source surface.
 * @param[in] srcRect Source rect.
 * @param[in] dst Destination surface.
 * @param[in] dstRect Destination rect.
 */
CTR_INLINE void kygxMakeRectCopy(KYGXCmd* cmd, const KYGXSurface* src, const KYGXRect* srcRect, const KYGXSurface* dst, const KYGXRect* dstRect) {
    CTR_ASSERT(cmd);
    CTR_ASSERT(src);
    CTR_ASSERT(srcRect);
    CTR_ASSERT(dst);
    CTR_ASSERT(dstRect);

    size_t srcOffset = 0;
    size_t srcSize = 0;
    uint16_t srcLineWidth = 0;
    uint16_t srcGap = 0;
    kygxGetRectCopyParams(src, srcRect, &srcOffset, &srcSize, &srcLineWidth, &srcGap);

    size_t dstOffset = 0;
    size_t dstSize = 0;
    uint16_t dstLineWidth = 0;
    uint16_t dstGap = 0;
    kygxGetRectCopyParams(dst, dstRect, &dstOffset, &dstSize, &dstLineWidth, &dstGap);

    CTR_ASSERT(srcSize == dstSize);

    kygxMakeTextureCopy(cmd, (const uint8_t*)src->addr + srcOffset, (uint8_t*)dst->addr + dstOffset, srcSize, srcLineWidth, srcGap, dstLineWidth, dstGap);
}

/**
 * @brief Execute TextureCopy synchronously.
 * @param[in] src Source buffer.
 * @param[in] dst Destination buffer.
 * @param[in] size Size.
 * @param[in] srcLineWidth Source line width.
 * @param[in] srcGap Source gap.
 * @param[in] dstLineWidth Destination line width.
 * @param[in] dstGap Destination gap.
 */
CTR_INLINE void kygxSyncTextureCopy(const void* src, void* dst, size_t size, uint16_t srcLineWidth, uint16_t srcGap, uint16_t dstLineWidth, uint16_t dstGap) {
    CTR_ASSERT(src);
    CTR_ASSERT(dst);
    
    KYGXCmd cmd;
    kygxMakeTextureCopy(&cmd, src, dst, size, srcLineWidth, srcGap, dstLineWidth, dstGap);
    kygxExecSync(&cmd);
}

/**
 * @brief Execute TextureCopy synchronously.
 * This wrapper can be used to copy raw buffers.
 * @param[in] src Source buffer.
 * @param[in] dst Destination buffer.
 * @param[in] size Buffer size.
 */
CTR_INLINE void kygxSyncMemoryCopy(const void* src, void* dst, size_t size) {
    CTR_ASSERT(src);
    CTR_ASSERT(dst);

    KYGXCmd cmd;
    kygxMakeMemoryCopy(&cmd, src, dst, size);
    kygxExecSync(&cmd);
}

/**
 * @brief Execute TextureCopy synchronously.
 * This wrapper can be used to copy surfaces.
 * @param[in] src Source surface.
 * @param[in] dst Destination surface.
 */
CTR_INLINE void kygxSyncSurfaceCopy(const KYGXSurface* src, const KYGXSurface* dst) {
    CTR_ASSERT(src);
    CTR_ASSERT(dst);

    KYGXCmd cmd;
    kygxMakeSurfaceCopy(&cmd, src, dst);
    kygxExecSync(&cmd);
}

/**
 * @brief Execute TextureCopy synchronously.
 * This wrapper can be used to copy rects between surfaces.
 * @param[in] src Source surface.
 * @param[in] srcRect Source rect.
 * @param[in] dst Destination surface.
 * @param[in] dstRect Destination rect.
 */
CTR_INLINE void kygxSyncRectCopy(const KYGXSurface* src, const KYGXRect* srcRect, const KYGXSurface* dst, const KYGXRect* dstRect) {
    CTR_ASSERT(src);
    CTR_ASSERT(srcRect);
    CTR_ASSERT(dst);
    CTR_ASSERT(dstRect);
    
    KYGXCmd cmd;
    kygxMakeRectCopy(&cmd, src, srcRect, dst, dstRect);
    kygxExecSync(&cmd);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* GUARD_KYGX_COMMAND_TEXTURECOPY_H */