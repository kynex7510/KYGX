#ifndef _CTRGX_COMMAND_BUFFER_H
#define _CTRGX_COMMAND_BUFFER_H

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

CTRGX_INLINE bool ctrgxCmdBufferAlloc(GXCmdBuffer* b, u8 capacity) {
    CTRGX_ASSERT(b);

    void* buffer = ctrgxAlloc(GX_MEM_HEAP, (sizeof(GXCmd) + sizeof(GXCallback) + sizeof(void*)) * capacity);
    if (buffer) {
        b->cmdList = (GXCmd*)buffer;
        b->callbackList = (GXCallback*)(&b->cmdList[capacity]);
        b->userDataList = (void**)(&b->callbackList[capacity]);
        b->capacity = capacity;
        return true;
    }

    return false;
}

CTRGX_INLINE void ctrgxCmdBufferFree(GXCmdBuffer* b) {
    CTRGX_ASSERT(b);

    ctrgxFree(b->cmdList);
    b->cmdList = NULL;
    b->callbackList = NULL;
    b->userDataList = NULL;
}

CTRGX_INLINE void ctrgxCmdBufferClear(GXCmdBuffer* b) {
    CTRGX_ASSERT(b);

    b->count = 0;
    b->index = 0;
}

CTRGX_INLINE bool ctrgxCmdBufferAdd(GXCmdBuffer* b, const GXCmd* cmd) {
    CTRGX_ASSERT(b);
    CTRGX_ASSERT(cmd);

    if (b->count >= b->capacity)
        return false;

    memcpy(&b->cmdList[(b->count + b->index) % b->capacity], cmd, sizeof(GXCmd));
    ++b->count;
    return true;
}

CTRGX_INLINE void ctrgxCmdBufferFinalize(GXCmdBuffer* b, GXCallback cb, void* cbData) {
    CTRGX_ASSERT(b);

    if (b->count) {
        const u8 idx = (b->count + b->index - 1) % b->capacity;
        GXCmd* lastCmd = &b->cmdList[idx];
        lastCmd->header |= CTRGX_CMDHEADER_FLAG_LAST;

        b->callbackList[idx] = cb;
        b->userDataList[idx] = cbData;
    }
}

CTRGX_INLINE bool ctrgxCmdBufferIsFinalized(GXCmdBuffer* b) {
    CTRGX_ASSERT(b);

    for (size_t i = b->count; i > 0; --i) {
        const GXCmd* cmd = &b->cmdList[((i - 1) + b->index) % b->capacity];
        if (cmd->header & CTRGX_CMDHEADER_FLAG_LAST)
            return true;
    }

    return false;
}

CTRGX_INLINE bool ctrgxCmdBufferPeek(GXCmdBuffer* b, u8 index, GXCmd** cmd, GXCallback* cb, void** cbData) {
    CTRGX_ASSERT(b);

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

CTRGX_INLINE void ctrgxCmdBufferAdvance(GXCmdBuffer* b, u8 size) {
    CTRGX_ASSERT(b);

    if (size > b->count)
        size = b->count;

    b->index += size;
    b->count -= size;
}

#endif /* _CTRGX_COMMAND_BUFFER_H */