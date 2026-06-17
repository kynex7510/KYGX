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

#define KYGX_DISPLAYTRANSFER_MODE_T2L 0
#define KYGX_DISPLAYTRANSFER_MODE_L2T (1 << 1)
#define KYGX_DISPLAYTRANSFER_MODE_T2T (1 << 5)

#define KYGX_DISPLAYTRANSFER_FMT_RGBA8 0
#define KYGX_DISPLAYTRANSFER_FMT_RGB8 1
#define KYGX_DISPLAYTRANSFER_FMT_RGB565 2
#define KYGX_DISPLAYTRANSFER_FMT_RGB5A1 3
#define KYGX_DISPLAYTRANSFER_FMT_RGBA4 4

#define KYGX_DISPLAYTRANSFER_DOWNSCALE_NONE 0
#define KYGX_DISPLAYTRANSFER_DOWNSCALE_2X1 1
#define KYGX_DISPLAYTRANSFER_DOWNSCALE_2X2 2

#define KYGX_DISPLAYTRANSFER_FLAG_MODE(v) (v)
#define KYGX_DISPLAYTRANSFER_FLAG_VERTICAL_FLIP (1 << 0)
#define KYGX_DISPLAYTRANSFER_FLAG_SRC_FORMAT(fmt) ((fmt) << 8)
#define KYGX_DISPLAYTRANSFER_FLAG_DST_FORMAT(fmt) ((fmt) << 12)
#define KYGX_DISPLAYTRANSFER_FLAG_BLOCKMODE32 (1 << 16)
#define KYGX_DISPLAYTRANSFER_FLAG_DOWNSCALE(v) ((v) << 24)

typedef struct {
    uint8_t mode;
    uint8_t srcFmt;
    uint8_t dstFmt;
    uint8_t downscale;
    bool verticalFlip;
    bool blockMode32;
} KYGXDisplayTransferFlags;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

CTR_INLINE uint32_t kygxGetDisplayTransferFlags(const KYGXDisplayTransferFlags* flags) {
    CTR_ASSERT(flags);

    CTR_ASSERT(flags->mode == KYGX_DISPLAYTRANSFER_MODE_T2L ||
        flags->mode == KYGX_DISPLAYTRANSFER_MODE_L2T ||
        flags->mode == KYGX_DISPLAYTRANSFER_MODE_T2T);
    uint32_t ret = KYGX_DISPLAYTRANSFER_FLAG_MODE(flags->mode);

    CTR_ASSERT(flags->srcFmt <= KYGX_DISPLAYTRANSFER_FMT_RGBA4);
    ret |= KYGX_DISPLAYTRANSFER_FLAG_SRC_FORMAT(flags->srcFmt);

    CTR_ASSERT(flags->dstFmt <= KYGX_DISPLAYTRANSFER_FMT_RGBA4);
    ret |= KYGX_DISPLAYTRANSFER_FLAG_DST_FORMAT(flags->dstFmt);

    CTR_ASSERT(flags->downscale <= KYGX_DISPLAYTRANSFER_DOWNSCALE_2X2);
    ret |= KYGX_DISPLAYTRANSFER_FLAG_DOWNSCALE(flags->downscale);

    if (flags->verticalFlip)
        ret |= KYGX_DISPLAYTRANSFER_FLAG_VERTICAL_FLIP;

    if (flags->blockMode32)
        ret |= KYGX_DISPLAYTRANSFER_FLAG_BLOCKMODE32;

    return ret;
}

CTR_INLINE void kygxMakeDisplayTransfer(KYGXCmd* cmd, const void* src, void* dst, uint16_t srcWidth, uint16_t srcHeight, uint16_t dstWidth, uint16_t dstHeight, uint32_t flags) {
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

CTR_INLINE bool kygxCheckDisplayTransferParams(uint16_t srcWidth, uint16_t srcHeight, uint16_t dstWidth, uint16_t dstHeight, const KYGXDisplayTransferFlags* flags) {
    CTR_ASSERT(flags);
    
    // Handle tiled -> linear mode.
    // TODO: test block mode 32.
    if (flags->mode == KYGX_DISPLAYTRANSFER_MODE_T2L) {
        // RGBA8 can convert into any other format.
        if (flags->srcFmt != KYGX_DISPLAYTRANSFER_FMT_RGBA8) {
            // RGB8 can only convert into itself.
            if (flags->srcFmt == KYGX_DISPLAYTRANSFER_FMT_RGB8) {
                if (flags->srcFmt != flags->dstFmt)
                    return false;
            } else {
                // other formats can only convert to other 16 bits formats.
                const bool isDst16 = flags->dstFmt == KYGX_DISPLAYTRANSFER_FMT_RGB565 ||
                    flags->dstFmt == KYGX_DISPLAYTRANSFER_FMT_RGB5A1 ||
                    flags->dstFmt == KYGX_DISPLAYTRANSFER_FMT_RGBA4;

                if (!isDst16)
                    return false;
            }
        }

        // Output dimensions must not be bigger than input ones.
        if (srcWidth < dstWidth || srcHeight < dstHeight)
            return false;

        // Width dimensions must be >= 64.
        if (srcWidth < 64 || dstWidth < 64)
            return false;

        // Height dimensions must be >= 16.
        if (srcHeight < 16 || dstHeight < 16)
            return false;

        // Width dimensions are required to be aligned to 16 bytes when doing RGB8 transfers.
        if (flags->srcFmt == KYGX_DISPLAYTRANSFER_FMT_RGB8) {
            if (!IsAligned(srcWidth, 16) || !IsAligned(dstWidth, 16))
                return false;
        } else {
            // Otherwise they are required to be aligned to 8 bytes.
            if (!IsAligned(srcWidth, 8) || !IsAligned(dstWidth, 8))
                return false;
        }

        // Check downscale.
        if (flags->downscale != KYGX_DISPLAYTRANSFER_DOWNSCALE_NONE) {
            // Input and output dimensions must be the same.
            if (srcWidth != dstWidth || srcHeight != dstHeight)
                return false;

            // Width/2 must also follow alignment constraints.
            const uint16_t wHalf = srcWidth / 2;
            if (flags->srcFmt == KYGX_DISPLAYTRANSFER_FMT_RGB8) {
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
    if (flags->mode == KYGX_DISPLAYTRANSFER_MODE_L2T) {
        // RGBA8, RGB8 can only convert to themselves.
        if (flags->srcFmt == KYGX_DISPLAYTRANSFER_FMT_RGBA8 || flags->srcFmt == KYGX_DISPLAYTRANSFER_FMT_RGB8) {
            if (flags->srcFmt != flags->dstFmt)
                return false;
        } else {
            // Other formats can convert to all other formats except RGB8.
            if (flags->dstFmt == KYGX_DISPLAYTRANSFER_FMT_RGB8)
                return false;
        }

        // TODO: dimension checks, downscale checks.
    }
    
    // Handle tiled -> tiled mode.
    // TODO: test block mode 32.
    if (flags->mode == KYGX_DISPLAYTRANSFER_MODE_T2T) {
        // Same as T2L.
        if (flags->srcFmt != KYGX_DISPLAYTRANSFER_FMT_RGBA8) {
            if (flags->srcFmt == KYGX_DISPLAYTRANSFER_FMT_RGB8) {
                if (flags->srcFmt != flags->dstFmt)
                    return false;
            } else {
                const bool isDst16 = flags->dstFmt == KYGX_DISPLAYTRANSFER_FMT_RGB565 ||
                    flags->dstFmt == KYGX_DISPLAYTRANSFER_FMT_RGB5A1 ||
                    flags->dstFmt == KYGX_DISPLAYTRANSFER_FMT_RGBA4;

                if (!isDst16)
                    return false;
            }
        }
        
        // Output dimensions should not be bigger than input ones.
        if (srcWidth < dstWidth || srcHeight < dstHeight)
            return false;

        // Width dimensions must be >= 64.
        if (srcWidth < 64 || dstWidth < 64)
            return false;

        // Height dimensions must be >= 32.
        if (srcHeight < 32 || dstHeight < 32)
            return false;

        // Width dimensions are required to be aligned to 64 bytes when doing RGBA8/RGB8 transfers.
        if (flags->srcFmt == KYGX_DISPLAYTRANSFER_FMT_RGBA8 || flags->srcFmt == KYGX_DISPLAYTRANSFER_FMT_RGB8) {
            if (!IsAligned(srcWidth, 64) || !IsAligned(dstWidth, 64))
                return false;
        } else {
            // Otherwise they are required to be aligned to 128 bytes.
            if (!IsAligned(srcWidth, 128) || !IsAligned(dstWidth, 128))
                return false;
        }

        // 2x2 downscale must be set.
        if (flags->downscale != KYGX_DISPLAYTRANSFER_DOWNSCALE_2X2)
            return false;
    }

    return true;
}

CTR_INLINE void kygxMakeDisplayTransferChecked(KYGXCmd* cmd, const void* src, void* dst, uint16_t srcWidth, uint16_t srcHeight, uint16_t dstWidth, uint16_t dstHeight, const KYGXDisplayTransferFlags* flags) {
    CTR_ASSERT(cmd);
    CTR_ASSERT(flags);
    CTR_ASSERT(kygxCheckDisplayTransferParams(srcWidth, srcHeight, dstWidth, dstHeight, flags));
    kygxMakeDisplayTransfer(cmd, src, dst, srcWidth, srcHeight, dstWidth, dstHeight, kygxGetDisplayTransferFlags(flags));
}

CTR_INLINE void kygxSyncDisplayTransfer(const void* src, void* dst, uint16_t srcWidth, uint16_t srcHeight, uint16_t dstWidth, uint16_t dstHeight, uint32_t flags) {
    KYGXCmd cmd;
    kygxMakeDisplayTransfer(&cmd, src, dst, srcWidth, srcHeight, dstWidth, dstHeight, flags);
    kygxExecSync(&cmd);
}

CTR_INLINE void kygxSyncDisplayTransferChecked(const void* src, void* dst, uint16_t srcWidth, uint16_t srcHeight, uint16_t dstWidth, uint16_t dstHeight, const KYGXDisplayTransferFlags* flags) {
    KYGXCmd cmd;
    kygxMakeDisplayTransferChecked(&cmd, src, dst, srcWidth, srcHeight, dstWidth, dstHeight, flags);
    kygxExecSync(&cmd);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* GUARD_KYGX_COMMAND_DISPLAYTRANSFER_H */