/**
 * @file DisplayTransfer.h
 * @brief Implementation of the DisplayTransfer command.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_KYGX_COMMAND_DISPLAYTRANSFER_H
#define GUARD_KYGX_COMMAND_DISPLAYTRANSFER_H

#include <CTR11/Assert.h>
#include <CTR11/Align.h>

#include <KYGX/GX.h>

typedef enum {
    KYGXTransferFormat_RGBA8 = 0,
    KYGXTransferFormat_RGB8 = 1,
    KYGXTransferFormat_RGB565 = 2,
    KYGXTransferFormat_RGB5A1 = 3,
    KYGXTransferFormat_RGBA4 = 4,
} KYGXTransferFormat;

typedef struct {
    void* addr;
    uint16_t width;
    uint16_t height;
    KYGXTransferFormat format;
} KYGXTransferSurface;

typedef enum {
    KYGXTransferMode_TiledToLinear = 0,
    KYGXTransferMode_LinearToTiled = (1 << 1),
    KYGXtransferMode_TiledToTiled = (1 << 5),
} KYGXTransferMode;

typedef enum {
    KYGXTransferDownscale_None = 0,
    KYGXTransferDownscale_2x1 = 1,
    KYGXTransferDownscale_2x2 = 2,
} KYGXTransferDownscale;

typedef enum {
    KYGXTransferFlip_None = 0,
    KYGXTransferFlip_Vertical = (1 << 0),
} KYGXTransferFlip;

typedef enum {
    KYGXTransferBlockMode_8 = 0,
    KYGXtransferBlockMode_32 = (1 << 16),
} KYGXTransferBlockMode;

typedef struct {
    KYGXTransferMode mode;
    KYGXTransferDownscale downscale;
    KYGXTransferFlip flip;
    KYGXTransferBlockMode blockMode;
} KYGXTransferFlags;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

CTR_INLINE uint32_t kygxPackDisplayTransferFlags(KYGXTransferFormat srcFmt, KYGXTransferFormat dstFmt, const KYGXTransferFlags* flags) {
    CTR_ASSERT(flags);
    
    uint32_t ret = (uint32_t)flags->mode;
    ret |= ((uint32_t)srcFmt << 8);
    ret |= ((uint32_t)dstFmt << 12);
    ret |= ((uint32_t)flags->downscale << 24);
    ret |= (uint32_t)flags->flip;
    ret |= (uint32_t)flags->blockMode;

    return ret;
}

CTR_INLINE void kygxMakeDisplayTransferRaw(KYGXCmd* cmd, const void* src, void* dst, uint16_t srcWidth, uint16_t srcHeight, uint16_t dstWidth, uint16_t dstHeight, uint32_t flags) {
    CTR_ASSERT(cmd);

    // Set crop bit.
    if (dstWidth < srcWidth)
        flags |= 0x4;

    cmd->header = KYGX_CMD_DISPLAYTRANSFER;

    cmd->params[0] = (uint32_t)src;
    cmd->params[1] = (uint32_t)dst;
    cmd->params[2] = (srcHeight << 16) | srcWidth;
    cmd->params[3] = (dstHeight << 16) | dstWidth;
    cmd->params[4] = flags & ~0x8u; // clear TextureCopy bit.
}

CTR_INLINE bool kygxCheckDisplayTransferParams(const KYGXTransferSurface* src, const KYGXTransferSurface* dst, const KYGXTransferFlags* flags) {
    CTR_ASSERT(src);
    CTR_ASSERT(dst);
    CTR_ASSERT(flags);
    
    // Handle tiled -> linear mode.
    // TODO: test block mode 32.
    if (flags->mode == KYGXTransferMode_TiledToLinear) {
        // RGBA8 can convert into any other format.
        if (src->format != KYGXTransferFormat_RGBA8) {
            // RGB8 can only convert into itself.
            if (src->format == KYGXTransferFormat_RGB8) {
                if (src->format != dst->format)
                    return false;
            } else {
                // other formats can only convert to other 16 bits formats.
                const bool isDst16 = dst->format == KYGXTransferFormat_RGB565 ||
                    dst->format == KYGXTransferFormat_RGB5A1 ||
                    dst->format == KYGXTransferFormat_RGBA4;

                if (!isDst16)
                    return false;
            }
        }

        // Output dimensions must not be bigger than input ones.
        if (src->width < dst->width || src->height < dst->height)
            return false;

        // Width dimensions must be >= 64.
        if (src->width < 64 || dst->width < 64)
            return false;

        // Height dimensions must be >= 16.
        if (src->height < 16 || dst->height < 16)
            return false;

        // Width dimensions are required to be aligned to 16 bytes when doing RGB8 transfers.
        if (src->format == KYGXTransferFormat_RGB8) {
            if (!IsAligned(src->width, 16) || !IsAligned(dst->width, 16))
                return false;
        } else {
            // Otherwise they are required to be aligned to 8 bytes.
            if (!IsAligned(src->width, 8) || !IsAligned(dst->width, 8))
                return false;
        }

        // Check downscale.
        if (flags->downscale != KYGXTransferDownscale_None) {
            // Input and output dimensions must be the same.
            if (src->width != dst->width || src->height != dst->height)
                return false;

            // Width/2 must also follow alignment constraints.
            const uint16_t wHalf = src->width / 2;
            if (src->format == KYGXTransferFormat_RGB8) {
                if (!IsAligned(wHalf, 16))
                    return false;
            } else {
                if (!IsAligned(wHalf, 8))
                    return false;
            }
        }
    }

    // Handle linear -> tiled mode.
    // TODO: test block mode 32.
    if (flags->mode == KYGXTransferMode_LinearToTiled) {
        // RGBA8, RGB8 can only convert to themselves.
        if (src->format == KYGXTransferFormat_RGBA8 || src->format == KYGXTransferFormat_RGB8) {
            if (src->format != dst->format)
                return false;
        } else {
            // Other formats can convert to all other formats except RGB8.
            if (dst->format == KYGXTransferFormat_RGB8)
                return false;
        }

        // TODO: dimension checks, downscale checks.
    }
    
    // Handle tiled -> tiled mode.
    // TODO: test block mode 32.
    if (flags->mode == KYGXtransferMode_TiledToTiled) {
        // Same as T2L.
        if (src->format != KYGXTransferFormat_RGBA8) {
            if (src->format == KYGXTransferFormat_RGB8) {
                if (src->format != dst->format)
                    return false;
            } else {
                const bool isDst16 = dst->format == KYGXTransferFormat_RGB565 ||
                    dst->format == KYGXTransferFormat_RGB5A1 ||
                    dst->format == KYGXTransferFormat_RGBA4;

                if (!isDst16)
                    return false;
            }
        }
        
        // Output dimensions should not be bigger than input ones.
        if (src->width < dst->width || src->height < dst->height)
            return false;

        // Width dimensions must be >= 64.
        if (src->width < 64 || dst->width < 64)
            return false;

        // Height dimensions must be >= 32.
        if (src->height < 32 || dst->height < 32)
            return false;

        // Width dimensions are required to be aligned to 64 bytes when doing RGBA8/RGB8 transfers.
        if (src->format == KYGXTransferFormat_RGBA8 || src->format == KYGXTransferFormat_RGB8) {
            if (!IsAligned(src->width, 64) || !IsAligned(dst->width, 64))
                return false;
        } else {
            // Otherwise they are required to be aligned to 128 bytes.
            if (!IsAligned(src->width, 128) || !IsAligned(dst->width, 128))
                return false;
        }

        // 2x2 downscale must be set.
        if (flags->downscale != KYGXTransferDownscale_2x2)
            return false;
    }

    return true;
}

CTR_INLINE void kygxMakeDisplayTransfer(KYGXCmd* cmd, const KYGXTransferSurface* src, const KYGXTransferSurface* dst, const KYGXTransferFlags* flags) {
    CTR_ASSERT(cmd);
    CTR_ASSERT(flags);
    CTR_ASSERT(kygxCheckDisplayTransferParams(src, dst, flags));

    const uint32_t packedFlags = kygxPackDisplayTransferFlags(src->format, dst->format, flags);
    kygxMakeDisplayTransferRaw(cmd, src->addr, dst->addr, src->width, src->height, dst->width, dst->height, packedFlags);
}

CTR_INLINE void kygxSyncDisplayTransfer(const KYGXTransferSurface* src, const KYGXTransferSurface* dst, const KYGXTransferFlags* flags) {
    CTR_ASSERT(src);
    CTR_ASSERT(dst);
    CTR_ASSERT(flags);
    
    KYGXCmd cmd;
    kygxMakeDisplayTransfer(&cmd, src, dst, flags);
    kygxExecSync(&cmd);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* GUARD_KYGX_COMMAND_DISPLAYTRANSFER_H */