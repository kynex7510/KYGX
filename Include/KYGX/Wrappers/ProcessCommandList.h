/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_KYGX_WRAPPERS_PROCESSCOMMANDLIST_H
#define GUARD_KYGX_WRAPPERS_PROCESSCOMMANDLIST_H

#include <CTR/Assert.h>

#include <KYGX/GX.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

CTR_INLINE void kygxMakeProcessCommandList(KYGXCmd* cmd, void* addr, size_t size, bool updateGasAccMax, bool flush) {
    CTR_ASSERT(cmd);

    cmd->header = KYGX_CMD_PROCESSCOMMANDLIST;
    cmd->params[0] = (uint32_t)addr;
    cmd->params[1] = size;
    cmd->params[2] = updateGasAccMax ? 1 : 0;
    cmd->params[3] = cmd->params[4] = cmd->params[5] = 0;
    cmd->params[6] = flush ? 1 : 0;
}

CTR_INLINE KYGXError kygxSyncProcessCommandList(void* addr, size_t size, bool updateGasAccMax, bool flush) {
    KYGXCmd cmd;
    kygxMakeProcessCommandList(&cmd, addr, size, updateGasAccMax, flush);
    return kygxExecSync(&cmd);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* GUARD_KYGX_WRAPPERS_PROCESSCOMMANDLIST_H */