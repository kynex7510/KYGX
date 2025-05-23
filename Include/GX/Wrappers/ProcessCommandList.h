#ifndef _CTRGX_HELPERS_PROCESSCOMMANDLIST_H
#define _CTRGX_HELPERS_PROCESSCOMMANDLIST_H

#include "GX/GX.h"

CTRGX_INLINE void ctrgxMakeProcessCommandList(GXCmd* cmd, void* addr, size_t size, bool updateGasAccMax, bool flush) {
    CTRGX_ASSERT(cmd);

    cmd->header = CTRGX_CMDID_PROCESSCOMMANDLIST;
    cmd->params[0] = (u32)addr;
    cmd->params[1] = size;
    cmd->params[2] = updateGasAccMax ? 1 : 0;
    cmd->params[3] = cmd->params[4] = cmd->params[5] = 0;
    cmd->params[6] = flush ? 1 : 0;
}

CTRGX_INLINE bool ctrgxAddProcessCommandList(GXCmdBuffer* b, void* addr, size_t size, bool updateGasAccMax, bool flush) {
    CTRGX_ASSERT(b);

    GXCmd cmd;
    ctrgxMakeProcessCommandList(&cmd, addr, size, updateGasAccMax, flush);
    return ctrgxCmdBufferAdd(b, &cmd);
}

CTRGX_INLINE void ctrgxSyncProcessCommandList(void* addr, size_t size, bool updateGasAccMax, bool flush) {
    GXCmd cmd;
    ctrgxMakeProcessCommandList(&cmd, addr, size, updateGasAccMax, flush);
    ctrgxExecSync(&cmd);
}

#endif /* _CTRGX_HELPERS_PROCESSCOMMANDLIST_H */