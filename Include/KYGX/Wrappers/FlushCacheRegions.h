/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_KYGX_WRAPPERS_FLUSHCACHEREGIONS_H
#define GUARD_KYGX_WRAPPERS_FLUSHCACHEREGIONS_H

#include <CTR/Assert.h>

#include <KYGX/GX.h>

typedef struct {
    const void* addr;
    size_t size;
} KYGXFlushCacheRegionsBuffer;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

CTR_INLINE void kygxMakeFlushCacheRegions(KYGXCmd* cmd, const KYGXFlushCacheRegionsBuffer* buffer0, const KYGXFlushCacheRegionsBuffer* buffer1, const KYGXFlushCacheRegionsBuffer* buffer2) {
    CTR_ASSERT(cmd);

    cmd->header = KYGX_CMD_FLUSHCACHEREGIONS;

    if (buffer0) {
        cmd->params[0] = (uint32_t)buffer0->addr;
        cmd->params[1] = buffer0->size;
    } else {
        cmd->params[0] = cmd->params[1] = 0;
    }

    if (buffer1) {
        cmd->params[2] = (uint32_t)buffer1->addr;
        cmd->params[3] = buffer1->size;
    } else {
        cmd->params[2] = cmd->params[3] = 0;
    }

    if (buffer2) {
        cmd->params[4] = (uint32_t)buffer2->addr;
        cmd->params[5] = buffer2->size;
    } else {
        cmd->params[4] = cmd->params[5] = 0;
    }

    cmd->params[6] = 0;
}

CTR_INLINE KYGXError kygxSyncFlushCacheRegions(const KYGXFlushCacheRegionsBuffer* buffer0, const KYGXFlushCacheRegionsBuffer* buffer1, const KYGXFlushCacheRegionsBuffer* buffer2) {
    KYGXCmd cmd;
    kygxMakeFlushCacheRegions(&cmd, buffer0, buffer1, buffer2);
    return kygxExecSync(&cmd);
}

CTR_INLINE KYGXError kygxSyncFlushSingleBuffer(const void* addr, size_t size) {
    KYGXFlushCacheRegionsBuffer flush;
    flush.addr = addr;
    flush.size = size;
    return kygxSyncFlushCacheRegions(&flush, NULL, NULL);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* GUARD_KYGX_WRAPPERS_FLUSHCACHEREGIONS_H */