/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _KYGX_WRAPPERS_DISPLAYTRANSFER_H
#define _KYGX_WRAPPERS_DISPLAYTRANSFER_H

#include <KYGX/GX.h>
#include <KYGX/Utility.h>

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
    u8 mode;
    u8 srcFmt;
    u8 dstFmt;
    u8 downscale;
    bool verticalFlip;
    bool blockMode32;
} KYGXDisplayTransferFlags;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

KYGX_INLINE u32 kygxGetDisplayTransferFlags(const KYGXDisplayTransferFlags* flags) {
    KYGX_ASSERT(flags);

    KYGX_ASSERT(flags->mode == KYGX_DISPLAYTRANSFER_MODE_T2L ||
        flags->mode == KYGX_DISPLAYTRANSFER_MODE_L2T ||
        flags->mode == KYGX_DISPLAYTRANSFER_MODE_T2T);
    u32 ret = KYGX_DISPLAYTRANSFER_FLAG_MODE(flags->mode);

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

KYGX_INLINE void kygxMakeDisplayTransfer(KYGXCmd* cmd, const void* src, void* dst, u16 srcWidth, u16 srcHeight, u16 dstWidth, u16 dstHeight, u32 flags) {
    KYGX_ASSERT(cmd);

    // Set crop bit.
    if (dstWidth < srcWidth)
        flags |= 0x4;

    cmd->header = KYGX_CMDID_DISPLAYTRANSFER;

    cmd->params[0] = (u32)src;
    cmd->params[1] = (u32)dst;
    cmd->params[2] = (srcHeight << 16) | srcWidth;
    cmd->params[3] = (dstHeight << 16) | dstWidth;
    cmd->params[4] = flags & ~0x8u; // clear TextureCopy bit.
}

KYGX_INLINE void kygxMakeDisplayTransferChecked(KYGXCmd* cmd, const void* src, void* dst, u16 srcWidth, u16 srcHeight, u16 dstWidth, u16 dstHeight, const KYGXDisplayTransferFlags* flags) {
    KYGX_ASSERT(cmd);
    KYGX_ASSERT(flags);

    // Handle tiled -> linear mode.
    // TODO: test block mode 32.
    if (flags->mode == KYGX_DISPLAYTRANSFER_MODE_T2L) {
        // RGBA8 can convert into any other format.
        if (flags->srcFmt != KYGX_DISPLAYTRANSFER_FMT_RGBA8) {
            // RGB8 can only convert into itself.
            if (flags->srcFmt == KYGX_DISPLAYTRANSFER_FMT_RGB8) {
                KYGX_ASSERT(flags->srcFmt == flags->dstFmt);
            } else {
                // other formats can only convert to other 16 bits formats.
                const bool isDst16 = flags->dstFmt == KYGX_DISPLAYTRANSFER_FMT_RGB565 ||
                    flags->dstFmt == KYGX_DISPLAYTRANSFER_FMT_RGB5A1 ||
                    flags->dstFmt == KYGX_DISPLAYTRANSFER_FMT_RGBA4;

                KYGX_ASSERT(isDst16);
            }
        }

        // Output dimensions must not be bigger than input ones.
        KYGX_ASSERT(srcWidth >= dstWidth && srcHeight >= dstHeight);

        // Width dimensions must be >= 64.
        KYGX_ASSERT(srcWidth >= 64 && dstWidth >= 64);

        // Height dimensions must be >= 16.
        KYGX_ASSERT(srcHeight >= 16 && dstHeight >= 16);

        // Width dimensions are required to be aligned to 16 bytes when doing RGB8 transfers.
        if (flags->srcFmt == KYGX_DISPLAYTRANSFER_FMT_RGB8) {
            KYGX_ASSERT(kygxIsAligned(srcWidth, 16) && kygxIsAligned(dstWidth, 16));
        } else {
             // Otherwise they are required to be aligned to 8 bytes.
            KYGX_ASSERT(kygxIsAligned(srcWidth, 8) && kygxIsAligned(dstWidth, 8));
        }

        // Check downscale.
        if (flags->downscale != KYGX_DISPLAYTRANSFER_DOWNSCALE_NONE) {
            // Input and output dimensions must be the same.
            KYGX_ASSERT(srcWidth == dstWidth && srcHeight == dstHeight);

            // Width/2 must also follow alignment constraints.
            const u16 wHalf = srcWidth / 2;
            if (flags->srcFmt == KYGX_DISPLAYTRANSFER_FMT_RGB8) {
                KYGX_ASSERT(kygxIsAligned(wHalf, 16));
            } else {
                KYGX_ASSERT(kygxIsAligned(wHalf, 8));
            }
        }
    }
    
    // Handle linear -> tiled mode.
    // TODO: test block mode 32.
    if (flags->mode == KYGX_DISPLAYTRANSFER_MODE_L2T) {
        // RGBA8, RGB8 can only convert to themselves.
        if (flags->srcFmt == KYGX_DISPLAYTRANSFER_FMT_RGBA8 || flags->srcFmt == KYGX_DISPLAYTRANSFER_FMT_RGB8) {
            KYGX_ASSERT(flags->srcFmt == flags->dstFmt);
        } else {
            // Other formats can convert to all other formats except RGB8.
            KYGX_ASSERT(flags->dstFmt != KYGX_DISPLAYTRANSFER_FMT_RGB8);
        }

        // TODO: dimension checks, downscale checks.
    }
    
    // Handle tiled -> tiled mode.
    // TODO: test block mode 32.
    if (flags->mode == KYGX_DISPLAYTRANSFER_MODE_T2T) {
        // Same as T2L.
        if (flags->srcFmt != KYGX_DISPLAYTRANSFER_FMT_RGBA8) {
            if (flags->srcFmt == KYGX_DISPLAYTRANSFER_FMT_RGB8) {
                KYGX_ASSERT(flags->srcFmt == flags->dstFmt);
            } else {
                const bool isDst16 = flags->dstFmt == KYGX_DISPLAYTRANSFER_FMT_RGB565 ||
                    flags->dstFmt == KYGX_DISPLAYTRANSFER_FMT_RGB5A1 ||
                    flags->dstFmt == KYGX_DISPLAYTRANSFER_FMT_RGBA4;

                KYGX_ASSERT(isDst16);
            }
        }

        // Output dimensions should not be bigger than input ones.
        KYGX_ASSERT(srcWidth >= dstWidth && srcHeight >= dstHeight);

        // Width dimensions must be >= 64.
        KYGX_ASSERT(srcWidth >= 64 && dstWidth >= 64);

        // Height dimensions must be >= 32.
        KYGX_ASSERT(srcHeight >= 32 && dstHeight >= 32);

        // Width dimensions are required to be aligned to 64 bytes when doing RGBA8/RGB8 transfers.
        if (flags->srcFmt == KYGX_DISPLAYTRANSFER_FMT_RGBA8 || flags->srcFmt == KYGX_DISPLAYTRANSFER_FMT_RGB8) {
            KYGX_ASSERT(kygxIsAligned(srcWidth, 64) && kygxIsAligned(dstWidth, 64));
        } else {
             // Otherwise they are required to be aligned to 128 bytes.
            KYGX_ASSERT(kygxIsAligned(srcWidth, 128) && kygxIsAligned(dstWidth, 128));
        }

        // 2x2 downscale must be set.
        KYGX_ASSERT(flags->downscale == KYGX_DISPLAYTRANSFER_DOWNSCALE_2X2);
    }

    kygxMakeDisplayTransfer(cmd, src, dst, srcWidth, srcHeight, dstWidth, dstHeight, kygxGetDisplayTransferFlags(flags));
}

KYGX_INLINE bool kygxAddDisplayTransfer(KYGXCmdBuffer* b, const void* src, void* dst, u16 srcWidth, u16 srcHeight, u16 dstWidth, u16 dstHeight, u32 flags) {
    KYGX_ASSERT(b);
    
    KYGXCmd cmd;
    kygxMakeDisplayTransfer(&cmd, src, dst, srcWidth, srcHeight, dstWidth, dstHeight, flags);
    return kygxCmdBufferAdd(b, &cmd);
}

KYGX_INLINE bool kygxAddDisplayTransferChecked(KYGXCmdBuffer* b, const void* src, void* dst, u16 srcWidth, u16 srcHeight, u16 dstWidth, u16 dstHeight, const KYGXDisplayTransferFlags* flags) {
    KYGX_ASSERT(b);
    KYGX_ASSERT(flags);
    
    KYGXCmd cmd;
    kygxMakeDisplayTransferChecked(&cmd, src, dst, srcWidth, srcHeight, dstWidth, dstHeight, flags);
    return kygxCmdBufferAdd(b, &cmd);
}

KYGX_INLINE void kygxSyncDisplayTransfer(const void* src, void* dst, u16 srcWidth, u16 srcHeight, u16 dstWidth, u16 dstHeight, u32 flags) {
    KYGXCmd cmd;
    kygxMakeDisplayTransfer(&cmd, src, dst, srcWidth, srcHeight, dstWidth, dstHeight, flags);
    kygxExecSync(&cmd);
}

KYGX_INLINE void kygxSyncDisplayTransferChecked(const void* src, void* dst, u16 srcWidth, u16 srcHeight, u16 dstWidth, u16 dstHeight, const KYGXDisplayTransferFlags* flags) {
    KYGXCmd cmd;
    kygxMakeDisplayTransferChecked(&cmd, src, dst, srcWidth, srcHeight, dstWidth, dstHeight, flags);
    kygxExecSync(&cmd);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _KYGX_WRAPPERS_DISPLAYTRANSFER_H */