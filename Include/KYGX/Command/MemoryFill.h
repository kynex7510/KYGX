/**
 * @file MemoryFill.h
 * @brief Implementation of the MemoryFill command.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_KYGX_COMMAND_MEMORYFILL_H
#define GUARD_KYGX_COMMAND_MEMORYFILL_H

#include <CTR11/Assert.h>
#include <CTR11/Align.h>
#include <CTR11/Memory.h>

#include <KYGX/GX.h>

/// @brief Construct an RGBA8 pixel.
#define KYGX_RGBA8_PIXEL(r, g, b, a) (((r) << 24) | ((g) << 16) | ((b) << 8) | (a))

/// @brief Construct an RGB8 pixel.
#define KYGX_RGB8_PIXEL(r, g, b) (((r) << 16) | ((g) << 8) | (b))

/// @brief Construct an RGB565 pixel.
#define KYGX_RGB565_PIXEL(r, g, b) ((((r) & 0x1F) << 11) | (((g) & 0x3F) << 5) | ((b) & 0x1F))

/// @brief Construct an RGB5A1 pixel.
#define KYGX_RGB5A1_PIXEL(r, g, b, a) ((((r) & 0x1F) << 11) | (((g) & 0x1F) << 6) | (((b) & 0x1F) << 1) | ((a) & 1))

/// @brief Construct an RGBA4 pixel.
#define KYGX_RGBA4_PIXEL(r, g, b, a) ((((r) & 0xF) << 12) | (((g) & 0xF) << 8) | (((b) & 0xF) << 4) | ((a) & 0xF))

/// @brief Width of the value used for the fill operation.
typedef enum {
    KYGXFillWidth_16Bits = 0, ///< The value is 16 bits (eg. RGB565, RGB5A1, RGBA4).
    KYGXFillWidth_24Bits = 1, ///< The value is 24 bits (eg. RGB8).
    KYGXFillWidth_32Bits = 2, ///< The value is 32 bits (eg. RGBA8).

    KYGXFillWidth_RGBA8 = KYGXFillWidth_32Bits,
    KYGXFillWidth_RGB8 = KYGXFillWidth_24Bits,
    KYGXFillWidth_RGB565 = KYGXFillWidth_16Bits,
    KYGXFillWidth_RGB5A1 = KYGXFillWidth_16Bits,
    KYGXFillWidth_RGBA4 = KYGXFillWidth_16Bits,
} KYGXFillWidth;

/// @brief Contains informations regarding a fill operation.
typedef struct {
    void* addr;          ///< Buffer address.
    size_t size;         ///< Buffer size.
    uint32_t value;      ///< Value used for the fill.
    KYGXFillWidth width; ///< Fill width.
} KYGXFill;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief Construct a MemoryFill command.
 * This command fills at most 2 memory regions with specific values. Only VRAM is supported, and
 * addresses and sizes must be 8 bytes aligned.
 * @param[out] cmd Output command.
 * @param[in] fill0 First fill.
 * @param[in] fill1 Second fill.
 */
CTR_INLINE void kygxMakeMemoryFill(KYGXCmd* cmd, const KYGXFill* fill0, const KYGXFill* fill1) {
    CTR_ASSERT(cmd);

    cmd->header = KYGX_CMD_MEMORYFILL;

    if (fill0 && fill0->size) {
        CTR_ASSERT(IsMemVRAM(fill0->addr, fill0->size));
        CTR_ASSERT(IsAligned((uint32_t)fill0->addr, 8));
        CTR_ASSERT(IsAligned(fill0->size, 8));

        cmd->params[0] = (uint32_t)fill0->addr;
        cmd->params[1] = fill0->value;
        cmd->params[2] = (uint32_t)((uint8_t*)fill0->addr + fill0->size);
        cmd->params[6] = ((uint32_t)fill0->width << 8) | 1;
    } else {
        cmd->params[0] = cmd->params[1] = cmd->params[2] = cmd->params[6] = 0;
    }

    if (fill1 && fill1->size) {
        CTR_ASSERT(IsMemVRAM(fill1->addr, fill1->size));
        CTR_ASSERT(IsAligned((uint32_t)fill1->addr, 8));
        CTR_ASSERT(IsAligned(fill1->size, 8));

        cmd->params[3] = (uint32_t)fill1->addr;
        cmd->params[4] = fill1->value;
        cmd->params[5] = (uint32_t)((uint8_t*)fill1->addr + fill1->size);
        cmd->params[6] |= (((uint32_t)fill1->width << 8) | 1) << 16;
    } else {
        cmd->params[3] = cmd->params[4] = cmd->params[5] = 0;
    }
}

/**
 * @brief Execute MemoryFill synchronously.
 * @param[in] fill0 First fill.
 * @param[in] fill1 Second fill.
 */
CTR_INLINE void kygxSyncMemoryFill(const KYGXFill* fill0, const KYGXFill* fill1) {
    KYGXCmd cmd;
    kygxMakeMemoryFill(&cmd, fill0, fill1);
    kygxExecSync(&cmd);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* GUARD_KYGX_COMMAND_MEMORYFILL_H */