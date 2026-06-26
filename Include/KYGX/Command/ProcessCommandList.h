/**
 * @file ProcessCommandList.h
 * @brief Implementation of the ProcessCommandList command.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_KYGX_COMMAND_PROCESSCOMMANDLIST_H
#define GUARD_KYGX_COMMAND_PROCESSCOMMANDLIST_H

#include <CTR11/Assert.h>
#include <CTR11/Align.h>
#include <CTR11/Allocator.h>

#include <KYGX/GX.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief Construct a ProcessCommandList command.
 * This command starts the processing of the specified GPU command list. Both address and size must be 8 bytes aligned.
 * @param[out] cmd Output command.
 * @param[in] addr GPU command list address.
 * @param[in] size GPU command list size.
 * @param[in] updateGasAccMax Whether to update gas additive results after the execution of the GPU command list.
 * @param[in] flush Whether to flush the source buffer before the command execution.
 */
CTR_INLINE void kygxMakeProcessCommandList(KYGXCmd* cmd, void* addr, size_t size, bool updateGasAccMax, bool flush) {
    CTR_ASSERT(cmd);
    CTR_ASSERT(IsAligned((uint32_t)addr, 8));
    CTR_ASSERT(IsAligned(size, 8));
    CTR_ASSERT(IsGPUAccessible(addr, size, MemAccess_Read));

    cmd->header = KYGX_CMD_PROCESSCOMMANDLIST;
    cmd->params[0] = (uint32_t)addr;
    cmd->params[1] = size;
    cmd->params[2] = updateGasAccMax ? 1 : 0;
    cmd->params[3] = cmd->params[4] = cmd->params[5] = 0;
    cmd->params[6] = flush ? 1 : 0;
}

/**
 * @brief Execute ProcessCommandList synchronously.
 * @param[in] addr GPU command list address.
 * @param[in] size GPU command list size.
 * @param[in] updateGasAccMax Whether to update gas additive results after the execution of the GPU command list.
 * @param[in] flush Whether to flush the source buffer before the command execution.
 */
CTR_INLINE void kygxSyncProcessCommandList(void* addr, size_t size, bool updateGasAccMax, bool flush) {
    KYGXCmd cmd;
    kygxMakeProcessCommandList(&cmd, addr, size, updateGasAccMax, flush);
    kygxExecSync(&cmd);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* GUARD_KYGX_COMMAND_PROCESSCOMMANDLIST_H */