/**
 * @file FlushCacheRegions.h
 * @brief Implementation of the FlushCacheRegions command.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_KYGX_COMMAND_FLUSHCACHEREGIONS_H
#define GUARD_KYGX_COMMAND_FLUSHCACHEREGIONS_H

#include <CTR11/Assert.h>

#include <KYGX/GX.h>

/// @brief Represents a memory region.
typedef struct {
    const void* addr; ///< Region address.
    size_t size;      ///< Region size.
} KYGXRegion;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief Construct a FlushCacheRegions command.
 * This command flushes the data cache of the specified regions. If a region has size 0 it's skipped,
 * in that case subsequent regions are not flushed aswell.
 * @param[out] cmd Output command.
 * @param[in] region0 First region.
 * @param[in] region1 Second region.
 * @param[in] region2 Third region.
 */
CTR_INLINE void kygxMakeFlushCacheRegions(KYGXCmd* cmd, const KYGXRegion* region0, const KYGXRegion* region1, const KYGXRegion* region2) {
    CTR_ASSERT(cmd);

    cmd->header = KYGX_CMD_FLUSHCACHEREGIONS;

    if (region0) {
        cmd->params[0] = (uint32_t)region0->addr;
        cmd->params[1] = region0->size;
    } else {
        cmd->params[0] = cmd->params[1] = 0;
    }

    if (region1) {
        cmd->params[2] = (uint32_t)region1->addr;
        cmd->params[3] = region1->size;
    } else {
        cmd->params[2] = cmd->params[3] = 0;
    }

    if (region2) {
        cmd->params[4] = (uint32_t)region2->addr;
        cmd->params[5] = region2->size;
    } else {
        cmd->params[4] = cmd->params[5] = 0;
    }

    cmd->params[6] = 0;
}

/**
 * @brief Execute FlushCacheRegions synchronously.
 * @param[in] region0 First region.
 * @param[in] region1 Second region.
 * @param[in] region2 Third region.
 */
CTR_INLINE void kygxSyncFlushCacheRegions(const KYGXRegion* region0, const KYGXRegion* region1, const KYGXRegion* region2) {
    KYGXCmd cmd;
    kygxMakeFlushCacheRegions(&cmd, region0, region1, region2);
    kygxExecSync(&cmd);
}
/**
 * @brief Execute FlushCacheRegions synchronously for a single region.
 * @param[in] addr Region address.
 * @param[in] size Region size.
 */
CTR_INLINE void kygxSyncFlushSingleRegion(const void* addr, size_t size) {
    KYGXRegion region;
    region.addr = addr;
    region.size = size;
    kygxSyncFlushCacheRegions(&region, NULL, NULL);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* GUARD_KYGX_COMMAND_FLUSHCACHEREGIONS_H */