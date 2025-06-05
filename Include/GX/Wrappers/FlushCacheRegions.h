#ifndef _KYGX_WRAPPERS_FLUSHCACHEREGIONS_H
#define _KYGX_WRAPPERS_FLUSHCACHEREGIONS_H

#include <GX/GX.h>

typedef struct {
    void* addr;
    size_t size;
} GXFlushCacheRegionsBuffer;

KYGX_INLINE void kygxMakeFlushCacheRegions(GXCmd* cmd, const GXFlushCacheRegionsBuffer* buffer0, const GXFlushCacheRegionsBuffer* buffer1, const GXFlushCacheRegionsBuffer* buffer2) {
    KYGX_ASSERT(cmd);

    cmd->header = KYGX_CMDID_FLUSHCACHEREGIONS;

    if (buffer0) {
        cmd->params[0] = (u32)buffer0->addr;
        cmd->params[1] = buffer0->size;
    } else {
        cmd->params[0] = cmd->params[1] = 0;
    }

    if (buffer1) {
        cmd->params[2] = (u32)buffer1->addr;
        cmd->params[3] = buffer1->size;
    } else {
        cmd->params[2] = cmd->params[3] = 0;
    }

    if (buffer2) {
        cmd->params[4] = (u32)buffer2->addr;
        cmd->params[5] = buffer2->size;
    } else {
        cmd->params[4] = cmd->params[5] = 0;
    }

    cmd->params[6] = 0;
}

// This command only exists in synchronous mode because it doesn't trigger any interrupt, and there is no nice way to know when it has completed.
KYGX_INLINE void kygxSyncFlushCacheRegions(const GXFlushCacheRegionsBuffer* buffer0, const GXFlushCacheRegionsBuffer* buffer1, const GXFlushCacheRegionsBuffer* buffer2) {
    GXCmd cmd;
    kygxMakeFlushCacheRegions(&cmd, buffer0, buffer1, buffer2);
    kygxExecSync(&cmd);
}

#endif /* _KYGX_WRAPPERS_FLUSHCACHEREGIONS_H */