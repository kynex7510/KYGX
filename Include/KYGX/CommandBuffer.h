#ifndef _KYGX_COMMAND_BUFFER_H
#define _KYGX_COMMAND_BUFFER_H

#include <KYGX/Command.h>
#include <KYGX/Allocator.h>

#include <string.h> // memcpy

typedef void (*KYGXCallback)(void* data);

typedef struct {
    KYGXCmd* cmdList;
    KYGXCallback* callbackList;
    void** userDataList;
    u8 count;
    u8 index;
    u8 capacity;
} KYGXCmdBuffer;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

KYGX_INLINE bool kygxCmdBufferAlloc(KYGXCmdBuffer* b, u8 capacity) {
    KYGX_ASSERT(b);

    void* buffer = kygxAlloc(KYGX_MEM_HEAP, (sizeof(KYGXCmd) + sizeof(KYGXCallback) + sizeof(void*)) * capacity);
    if (buffer) {
        b->cmdList = (KYGXCmd*)buffer;
        b->callbackList = (KYGXCallback*)(&b->cmdList[capacity]);
        b->userDataList = (void**)(&b->callbackList[capacity]);
        b->capacity = capacity;
        return true;
    }

    return false;
}

KYGX_INLINE void kygxCmdBufferFree(KYGXCmdBuffer* b) {
    KYGX_ASSERT(b);

    kygxFree(b->cmdList);
    b->cmdList = NULL;
    b->callbackList = NULL;
    b->userDataList = NULL;
}

KYGX_INLINE void kygxCmdBufferClear(KYGXCmdBuffer* b) {
    KYGX_ASSERT(b);

    b->count = 0;
    b->index = 0;
}

KYGX_INLINE bool kygxCmdBufferAdd(KYGXCmdBuffer* b, const KYGXCmd* cmd) {
    KYGX_ASSERT(b);
    KYGX_ASSERT(cmd);

    if (b->count >= b->capacity)
        return false;

    memcpy(&b->cmdList[(b->count + b->index) % b->capacity], cmd, sizeof(KYGXCmd));
    ++b->count;
    return true;
}

KYGX_INLINE void kygxCmdBufferFinalize(KYGXCmdBuffer* b, KYGXCallback cb, void* cbData) {
    KYGX_ASSERT(b);

    if (b->count) {
        const u8 idx = (b->count + b->index - 1) % b->capacity;
        KYGXCmd* lastCmd = &b->cmdList[idx];
        lastCmd->header |= KYGX_CMDHEADER_FLAG_LAST;

        b->callbackList[idx] = cb;
        b->userDataList[idx] = cbData;
    }
}

KYGX_INLINE bool kygxCmdBufferIsFinalized(KYGXCmdBuffer* b) {
    KYGX_ASSERT(b);

    for (size_t i = b->count; i > 0; --i) {
        const KYGXCmd* cmd = &b->cmdList[((i - 1) + b->index) % b->capacity];
        if (cmd->header & KYGX_CMDHEADER_FLAG_LAST)
            return true;
    }

    return false;
}

KYGX_INLINE bool kygxCmdBufferPeek(KYGXCmdBuffer* b, u8 index, KYGXCmd** cmd, KYGXCallback* cb, void** cbData) {
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

KYGX_INLINE void kygxCmdBufferAdvance(KYGXCmdBuffer* b, u8 size) {
    KYGX_ASSERT(b);

    if (size > b->count)
        size = b->count;

    b->index += size;
    b->count -= size;
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _KYGX_COMMAND_BUFFER_H */