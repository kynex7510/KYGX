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

#include <KYGX/GX.h>

#define KYGX_MEMORYFILL_VALUE_RGBA8(r, g, b, a) (((r) << 24) | ((g) << 16) | ((b) << 8) | (a))
#define KYGX_MEMORYFILL_VALUE_RGB8(r, g, b) (((r) << 16) | ((g) << 8) | (b))
#define KYGX_MEMORYFILL_VALUE_RGB565(r, g, b) ((((r) & 0x1F) << 11) | (((g) & 0x3F) << 5) | ((b) & 0x1F))
#define KYGX_MEMORYFILL_VALUE_RGB5A1(r, g, b, a) ((((r) & 0x1F) << 11) | (((g) & 0x1F) << 6) | (((b) & 0x1F) << 1) | ((a) & 1))
#define KYGX_MEMORYFILL_VALUE_RGBA4(r, g, b, a) ((((r) & 0xF) << 12) | (((g) & 0xF) << 8) | (((b) & 0xF) << 4) | ((a) & 0xF))

#define KYGX_MEMORYFILL_WIDTH_16 0
#define KYGX_MEMORYFILL_WIDTH_24 1
#define KYGX_MEMORYFILL_WIDTH_32 2

typedef struct {
    void* addr;
    size_t size;
    uint32_t value;
    uint8_t width;
} KYGXMemoryFillBuffer;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

CTR_INLINE void kygxMakeMemoryFill(KYGXCmd* cmd, const KYGXMemoryFillBuffer* buffer0, const KYGXMemoryFillBuffer* buffer1) {
    CTR_ASSERT(cmd);

    cmd->header = KYGX_CMD_MEMORYFILL;

    if (buffer0 && buffer0->addr) {
        cmd->params[0] = (uint32_t)buffer0->addr;
        cmd->params[1] = buffer0->value;
        cmd->params[2] = (uint32_t)((uint8_t*)buffer0->addr + buffer0->size);
        cmd->params[6] = (buffer0->width << 8) | 1;
    } else {
        cmd->params[0] = cmd->params[1] = cmd->params[2] = cmd->params[6] = 0;
    }

    if (buffer1 && buffer1->addr) {
        cmd->params[3] = (uint32_t)buffer1->addr;
        cmd->params[4] = buffer1->value;
        cmd->params[5] = (uint32_t)((uint8_t*)buffer1->addr + buffer1->size);
        cmd->params[6] |= ((buffer1->width << 8) | 1) << 16;
    } else {
        cmd->params[3] = cmd->params[4] = cmd->params[5] = 0;
    }
}

CTR_INLINE void kygxSyncMemoryFill(const KYGXMemoryFillBuffer* buffer0, const KYGXMemoryFillBuffer* buffer1) {
    KYGXCmd cmd;
    kygxMakeMemoryFill(&cmd, buffer0, buffer1);
    kygxExecSync(&cmd);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* GUARD_KYGX_COMMAND_MEMORYFILL_H */