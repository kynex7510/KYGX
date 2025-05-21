#include "GX/CommandBuffer.h"

#include <stdlib.h> // malloc, free

bool ctrgxCmdBufferAlloc(GXCmdBuffer* b, u8 capacity) {
    CTRGX_ASSERT(b);

    void* buffer = malloc((sizeof(GXCmd) + sizeof(GXCallback) + sizeof(void*)) * capacity);
    if (buffer) {
        b->cmdList = (GXCmd*)buffer;
        b->callbackList = (GXCallback*)(&b->cmdList[capacity]);
        b->userDataList = (void**)(&b->callbackList[capacity]);
        b->capacity = capacity;
        return true;
    }

    return false;
}

void ctrgxCmdBufferFree(GXCmdBuffer* b) {
    CTRGX_ASSERT(b);

    free(b->cmdList);
    b->cmdList = NULL;
    b->callbackList = NULL;
    b->userDataList = NULL;
}