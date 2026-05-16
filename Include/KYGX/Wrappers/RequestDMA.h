/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_KYGX_WRAPPERS_REQUESTDMA_H
#define GUARD_KYGX_WRAPPERS_REQUESTDMA_H

#include <CTR/Assert.h>

#include <KYGX/GX.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

CTR_INLINE void kygxMakeRequestDMA(KYGXCmd* cmd, const void* src, void* dst, size_t size, bool flush) {
    CTR_ASSERT(cmd);

    cmd->header = KYGX_CMD_REQUESTDMA;
    cmd->params[0] = (uint32_t)src;
    cmd->params[1] = (uint32_t)dst;
    cmd->params[2] = size;
    cmd->params[3] = cmd->params[4] = cmd->params[5] = 0;
    cmd->params[6] = flush ? 1 : 0;
}

CTR_INLINE KYGXError kygxSyncRequestDMA(const void* src, void* dst, size_t size, bool flush) {
    KYGXCmd cmd;
    kygxMakeRequestDMA(&cmd, src, dst, size, flush);
    return kygxExecSync(&cmd);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* GUARD_KYGX_WRAPPERS_REQUESTDMA_H */