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
#include <CTR11/Unreachable.h>

#include <KYGX/GX.h>

/// @brief Pixel format for a transfer buffer.
typedef enum {
    KYGXTransferFormat_RGBA8 = 0,  ///< RGBA8
    KYGXTransferFormat_RGB8 = 1,   ///< RGB8
    KYGXTransferFormat_RGB565 = 2, ///< RGB565
    KYGXTransferFormat_RGB5A1 = 3, ///< RGB5A1
    KYGXTransferFormat_RGBA4 = 4,  ///< RGBA4
} KYGXTransferFormat;

/// @brief Represents a pixel buffer in a transfer operation.
typedef struct {
    void* addr;                ///< Buffer address.
    uint16_t width;            ///< Buffer width.
    uint16_t height;           ///< Buffer height.
    KYGXTransferFormat format; ///< Pixel format.
} KYGXTransferBuffer;

/// @brief Mode of operation, each mode has different features and constraints.
typedef enum {
    KYGXTransferMode_TiledToLinear = 0,        ///< Convert the buffer from tiled/swizzled to linear.
    KYGXTransferMode_LinearToTiled = (1 << 1), ///< Convert the buffer from linear to tiled/swizzled.
    KYGXtransferMode_TiledToTiled = (1 << 5),  ///< Assume source and destination are swizzled, and don't do anything.
} KYGXTransferMode;

/// @brief Scales down the buffer using a box filter.
typedef enum {
    KYGXTransferDownscale_None = 0, ///< No downscale.
    KYGXTransferDownscale_2x1 = 1,  ///< 2x1 downscale, halves the buffer width.
    KYGXTransferDownscale_2x2 = 2,  ///< 2x2 downscale, halves the buffer dimensions.
} KYGXTransferDownscale;

/// @brief Controls buffer flipping.
typedef enum {
    KYGXTransferFlip_None = 0,            ///< Don't flip.
    KYGXTransferFlip_Vertical = (1 << 0), ///< Flip the buffer vertically.
} KYGXTransferFlip;

/// @brief Sets the size of a single tile.
typedef enum {
    KYGXTransferTileSize_8x8 = 0,           ///< 1 tile is 8x8.
    KYGXtransferTileSize_32x32 = (1 << 16), ///< 1 tile is 32x32.
} KYGXTransferTileSize;

/// @brief Transfer flags.
typedef struct {
    KYGXTransferMode mode;           ///< Mode of operation.
    KYGXTransferDownscale downscale; ///< Downscale.
    KYGXTransferFlip flip;           ///< Flip.
    KYGXTransferTileSize tileSize;   ///< Tile size.
} KYGXTransferFlags;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * Packs transfer flags into a word.
 * @param[in] srcFmt Source buffer format.
 * @param[in] dstFmt Destination buffer format.
 * @param[in] flags Transfer flags.
 * @return Packed transfer flags.
 */
CTR_INLINE uint32_t kygxPackDisplayTransferFlags(KYGXTransferFormat srcFmt, KYGXTransferFormat dstFmt, const KYGXTransferFlags* flags) {
    CTR_ASSERT(flags);
    
#ifndef NDEBUG

    switch (srcFmt) {
        case KYGXTransferFormat_RGBA8:
        case KYGXTransferFormat_RGB8:
        case KYGXTransferFormat_RGB565:
        case KYGXTransferFormat_RGB5A1:
        case KYGXTransferFormat_RGBA4:
            break;
        default:
            CTR_UNREACHABLE("Invalid source format");
    }

    switch (dstFmt) {
        case KYGXTransferFormat_RGBA8:
        case KYGXTransferFormat_RGB8:
        case KYGXTransferFormat_RGB565:
        case KYGXTransferFormat_RGB5A1:
        case KYGXTransferFormat_RGBA4:
            break;
        default:
            CTR_UNREACHABLE("Invalid destination format");
    }

    switch (flags->mode) {
        case KYGXTransferMode_TiledToLinear:
        case KYGXTransferMode_LinearToTiled:
        case KYGXtransferMode_TiledToTiled:
            break;
        default:
            CTR_UNREACHABLE("Invalid transfer mode");
    }

    switch (flags->downscale) {
        case KYGXTransferDownscale_None:
        case KYGXTransferDownscale_2x1:
        case KYGXTransferDownscale_2x2: 
            break;
        default:
            CTR_UNREACHABLE("Invalid downscale");
    }

    switch (flags->flip) {
        case KYGXTransferFlip_None:
        case KYGXTransferFlip_Vertical:
            break;
        default:
            CTR_UNREACHABLE("Invalid flip mode");
    }

    switch (flags->tileSize) {
        case KYGXTransferTileSize_8x8:
        case KYGXtransferTileSize_32x32:
            break;
        default:
            CTR_UNREACHABLE("Invalid tile size");
    }

#endif // !NDEBUG

    uint32_t ret = (uint32_t)flags->mode;
    ret |= ((uint32_t)srcFmt << 8);
    ret |= ((uint32_t)dstFmt << 12);
    ret |= ((uint32_t)flags->downscale << 24);
    ret |= (uint32_t)flags->flip;
    ret |= (uint32_t)flags->tileSize;

    return ret;
}

/**
 * @brief Construct a DisplayTransfer command.
 * This command has multiple functionalities, but is generally used for transferring GPU framebuffers into LCD framebuffers,
 * and for mipmap generation. Both source and destination addresses must be aligned to 8 bytes. Additional constraints
 * apply which depend on the performed operation. For more info, see \ref kygxCheckDisplayTransferParams.
 * @warning This function should not be used; use \ref kygxMakeDisplayTransfer instead.
 * @param[out] cmd Output command.
 * @param[in] src Source buffer.
 * @param[in] dst Destination buffer.
 * @param[in] srcWidth Source width.
 * @param[in] dstWidth Destination width.
 * @param[in] srcHeight Source Height.
 * @param[in] dstHeight Destination height.
 * @param[in] flags Transfer flags.
 */
CTR_INLINE void kygxMakeDisplayTransferRaw(KYGXCmd* cmd, const void* src, void* dst, uint16_t srcWidth, uint16_t srcHeight, uint16_t dstWidth, uint16_t dstHeight, uint32_t flags) {
    CTR_ASSERT(cmd);
    CTR_ASSERT(src);
    CTR_ASSERT(dst);
    CTR_ASSERT(IsAligned((uint32_t)src, 8));
    CTR_ASSERT(IsAligned((uint32_t)dst, 8));

    size_t srcPixelSize = 0;

    switch ((flags >> 8) & 0xF) {
        case KYGXTransferFormat_RGBA8:
            srcPixelSize = 4;
            break;
        case KYGXTransferFormat_RGB8:
            srcPixelSize = 3;
            break;
        case KYGXTransferFormat_RGB565:
        case KYGXTransferFormat_RGB5A1:
        case KYGXTransferFormat_RGBA4:
            srcPixelSize = 2;
            break;
        default:
            CTR_UNREACHABLE("Invalid format!");
    }

    CTR_ASSERT(IsGPUAccessible(src, srcWidth * srcHeight * srcPixelSize, MemAccess_Read));

    size_t dstPixelSize = 0;

    switch ((flags >> 12) & 0xF) {
        case KYGXTransferFormat_RGBA8:
            dstPixelSize = 4;
            break;
        case KYGXTransferFormat_RGB8:
            dstPixelSize = 3;
            break;
        case KYGXTransferFormat_RGB565:
        case KYGXTransferFormat_RGB5A1:
        case KYGXTransferFormat_RGBA4:
            dstPixelSize = 2;
            break;
        default:
            CTR_UNREACHABLE("Invalid format!");
    }

    CTR_ASSERT(IsGPUAccessible(dst, dstWidth * dstHeight * dstPixelSize, MemAccess_Write));

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

/**
 * @brief Checks whether a given combination of buffers and flags is accepted by the hardware.
 * @param[in] src Source buffer.
 * @param[in] dst Destination buffer.
 * @param[in] flags Transfer flags.
 * @return True if the combination is valid, false otherwise.
 */
CTR_INLINE bool kygxCheckDisplayTransferParams(const KYGXTransferBuffer* src, const KYGXTransferBuffer* dst, const KYGXTransferFlags* flags) {
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

/**
 * @brief Construct a DisplayTransfer command.
 * This wrapper simplifies command construction and performs checks when compiling in debug mode.
 * @param[out] cmd Output command.
 * @param[in] src Source buffer.
 * @param[in] dst Destination buffer.
 * @param[in] flags Transfer flags.
 */
CTR_INLINE void kygxMakeDisplayTransfer(KYGXCmd* cmd, const KYGXTransferBuffer* src, const KYGXTransferBuffer* dst, const KYGXTransferFlags* flags) {
    CTR_ASSERT(cmd);
    CTR_ASSERT(src);
    CTR_ASSERT(dst);
    CTR_ASSERT(flags);
    CTR_ASSERT(kygxCheckDisplayTransferParams(src, dst, flags));

    const uint32_t packedFlags = kygxPackDisplayTransferFlags(src->format, dst->format, flags);
    kygxMakeDisplayTransferRaw(cmd, src->addr, dst->addr, src->width, src->height, dst->width, dst->height, packedFlags);
}

/**
 * @brief Execute DisplayTransfer synchronously.
 * @param[in] src Source buffer.
 * @param[in] dst Destination buffer.
 * @param[in] flags Transfer flags.
 */
CTR_INLINE void kygxSyncDisplayTransfer(const KYGXTransferBuffer* src, const KYGXTransferBuffer* dst, const KYGXTransferFlags* flags) {
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