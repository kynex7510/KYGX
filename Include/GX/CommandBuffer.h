#ifndef _KYGX_COMMAND_BUFFER_H
#define _KYGX_COMMAND_BUFFER_H

#include <GX/Command.h>
#include <GX/Allocator.h>

#include <string.h> // memcpy

typedef void (*GXCallback)(void* data);

typedef struct {
    GXCmd* cmdList;
    GXCallback* callbackList;
    void** userDataList;
    u8 count;
    u8 index;
    u8 capacity;
} GXCmdBuffer;

KYGX_INLINE bool kygxCmdBufferAlloc(GXCmdBuffer* b, u8 capacity) {
    KYGX_ASSERT(b);

    void* buffer = kygxAlloc(GX_MEM_HEAP, (sizeof(GXCmd) + sizeof(GXCallback) + sizeof(void*)) * capacity);
    if (buffer) {
        b->cmdList = (GXCmd*)buffer;
        b->callbackList = (GXCallback*)(&b->cmdList[capacity]);
        b->userDataList = (void**)(&b->callbackList[capacity]);
        b->capacity = capacity;
        return true;
    }

    return false;
}

KYGX_INLINE void kygxCmdBufferFree(GXCmdBuffer* b) {
    KYGX_ASSERT(b);

    kygxFree(b->cmdList);
    b->cmdList = NULL;
    b->callbackList = NULL;
    b->userDataList = NULL;
}

KYGX_INLINE void kygxCmdBufferClear(GXCmdBuffer* b) {
    KYGX_ASSERT(b);

    b->count = 0;
    b->index = 0;
}

KYGX_INLINE bool kygxCmdBufferAdd(GXCmdBuffer* b, const GXCmd* cmd) {
    KYGX_ASSERT(b);
    KYGX_ASSERT(cmd);

    if (b->count >= b->capacity)
        return false;

    memcpy(&b->cmdList[(b->count + b->index) % b->capacity], cmd, sizeof(GXCmd));
    ++b->count;
    return true;
}

KYGX_INLINE void kygxCmdBufferFinalize(GXCmdBuffer* b, GXCallback cb, void* cbData) {
    KYGX_ASSERT(b);

    if (b->count) {
        const u8 idx = (b->count + b->index - 1) % b->capacity;
        GXCmd* lastCmd = &b->cmdList[idx];
        lastCmd->header |= KYGX_CMDHEADER_FLAG_LAST;

        b->callbackList[idx] = cb;
        b->userDataList[idx] = cbData;
    }
}

KYGX_INLINE bool kygxCmdBufferIsFinalized(GXCmdBuffer* b) {
    KYGX_ASSERT(b);

    for (size_t i = b->count; i > 0; --i) {
        const GXCmd* cmd = &b->cmdList[((i - 1) + b->index) % b->capacity];
        if (cmd->header & KYGX_CMDHEADER_FLAG_LAST)
            return true;
    }

    return false;
}

KYGX_INLINE bool kygxCmdBufferPeek(GXCmdBuffer* b, u8 index, GXCmd** cmd, GXCallback* cb, void** cbData) {
    KYGX_ASSERT(b);

    if (index >= b->count)
        return false;

    const u8 idx = (b->index + index) % b->capacity;

    if (cmd)
        *cmd = &b->cmdList[idx];
    
    if (cb)
        *cb = b->callbackList[idx];

    if (cbData)
        *cbData = b->userDataList[idx];

    return true;
}

KYGX_INLINE void kygxCmdBufferAdvance(GXCmdBuffer* b, u8 size) {
    KYGX_ASSERT(b);

    if (size > b->count)
        size = b->count;

    b->index += size;
    b->count -= size;
}

#endif /* _KYGX_COMMAND_BUFFER_H */