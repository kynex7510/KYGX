#ifndef _KYGX_WRAPPERS_PROCESSCOMMANDLIST_H
#define _KYGX_WRAPPERS_PROCESSCOMMANDLIST_H

#include <GX/GX.h>

KYGX_INLINE void kygxMakeProcessCommandList(GXCmd* cmd, void* addr, size_t size, bool updateGasAccMax, bool flush) {
    KYGX_ASSERT(cmd);

    cmd->header = KYGX_CMDID_PROCESSCOMMANDLIST;
    cmd->params[0] = (u32)addr;
    cmd->params[1] = size;
    cmd->params[2] = updateGasAccMax ? 1 : 0;
    cmd->params[3] = cmd->params[4] = cmd->params[5] = 0;
    cmd->params[6] = flush ? 1 : 0;
}

KYGX_INLINE bool kygxAddProcessCommandList(GXCmdBuffer* b, void* addr, size_t size, bool updateGasAccMax, bool flush) {
    KYGX_ASSERT(b);

    GXCmd cmd;
    kygxMakeProcessCommandList(&cmd, addr, size, updateGasAccMax, flush);
    return kygxCmdBufferAdd(b, &cmd);
}

KYGX_INLINE void kygxSyncProcessCommandList(void* addr, size_t size, bool updateGasAccMax, bool flush) {
    GXCmd cmd;
    kygxMakeProcessCommandList(&cmd, addr, size, updateGasAccMax, flush);
    kygxExecSync(&cmd);
}

#endif /* _KYGX_WRAPPERS_PROCESSCOMMANDLIST_H */