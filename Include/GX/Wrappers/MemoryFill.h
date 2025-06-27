#ifndef _KYGX_WRAPPERS_MEMORYFILL_H
#define _KYGX_WRAPPERS_MEMORYFILL_H

#include <GX/GX.h>

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
    u32 value;
    u8 width;
} GXMemoryFillBuffer;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

KYGX_INLINE void kygxMakeMemoryFill(KYGXCmd* cmd, const GXMemoryFillBuffer* buffer0, const GXMemoryFillBuffer* buffer1) {
    KYGX_ASSERT(cmd);

    cmd->header = KYGX_CMDID_MEMORYFILL;

    if (buffer0 && buffer0->addr) {
        cmd->params[0] = (u32)buffer0->addr;
        cmd->params[1] = buffer0->value;
        cmd->params[2] = (u32)((u8*)buffer0->addr + buffer0->size);
        cmd->params[6] = (buffer0->width << 8) | 1;
    } else {
        cmd->params[0] = cmd->params[1] = cmd->params[2] = cmd->params[6] = 0;
    }

    if (buffer1 && buffer1->addr) {
        cmd->params[3] = (u32)buffer1->addr;
        cmd->params[4] = buffer1->value;
        cmd->params[5] = (u32)((u8*)buffer1->addr + buffer1->size);
        cmd->params[6] |= ((buffer1->width << 8) | 1) << 16;
    } else {
        cmd->params[3] = cmd->params[4] = cmd->params[5] = 0;
    }
}

KYGX_INLINE bool kygxAddMemoryFill(KYGXCmdBuffer* b, const GXMemoryFillBuffer* buffer0, const GXMemoryFillBuffer* buffer1) {
    KYGX_ASSERT(b);
    
    KYGXCmd cmd;
    kygxMakeMemoryFill(&cmd, buffer0, buffer1);
    return kygxCmdBufferAdd(b, &cmd);    
}

KYGX_INLINE void kygxSyncMemoryFill(const GXMemoryFillBuffer* buffer0, const GXMemoryFillBuffer* buffer1) {
    KYGXCmd cmd;
    kygxMakeMemoryFill(&cmd, buffer0, buffer1);
    kygxExecSync(&cmd);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _KYGX_WRAPPERS_MEMORYFILL_H */