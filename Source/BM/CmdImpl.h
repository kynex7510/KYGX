/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <util.h>
#include <drivers/cache.h>
#include <arm11/drivers/gx.h>
#include <arm11/drivers/gpu_regs.h>

#include "../State.h"

KYGX_INLINE void doProcessCommandList(u32 addr, u32 size, bool updateGasAccMax, bool flush) {
    // TODO: gas

    if (flush)
        flushDCacheRange((void*)addr, size);

    GxRegs* regs = getGxRegs();
    regs->p3d[GPUREG_IRQ_ACK] = 0;

    while (regs->psc_irq_stat & IRQ_STAT_P3D)
        wait_cycles(16);

    regs->p3d[GPUREG_CMDBUF_SIZE0] = size >> 3;
    regs->p3d[GPUREG_CMDBUF_ADDR0] = addr >> 3;
    regs->p3d[GPUREG_CMDBUF_JUMP0] = 1;
}

KYGX_INLINE void doMemoryFill(u32 buf0s, u32 buf0v, u32 buf0e, u32 buf1s, u32 buf1v, u32 buf1e, u32 ctl) {
    GxRegs* regs = getGxRegs();

    if (buf0s) {
        regs->psc_fill0.s_addr = buf0s >> 3;
        regs->psc_fill0.e_addr = buf0e >> 3;
        regs->psc_fill0.val = buf0v;
        regs->psc_fill0.cnt = (regs->psc_fill0.cnt & 0xFFFF0000) | (ctl & 0xFFFF);
    }

    if (buf1s) {
        regs->psc_fill1.s_addr = buf1s >> 3;
        regs->psc_fill1.e_addr = buf1e >> 3;
        regs->psc_fill1.val = buf1v;
        regs->psc_fill1.cnt = (regs->psc_fill1.cnt & 0xFFFF0000) | (ctl >> 16);
    }
}

KYGX_INLINE void doDisplayTransfer(u32 src, u32 dst, u32 srcDim, u32 dstDim, u32 flags) {
    GxRegs* regs = getGxRegs();

    regs->ppf.in_addr = src >> 3;
    regs->ppf.out_addr = dst >> 3;
    regs->ppf.dt_outdim = dstDim;
    regs->ppf.dt_indim = srcDim;
    regs->ppf.flags = flags;
    regs->ppf.unk14 = 0;
    regs->ppf.cnt = PPF_EN;
}

KYGX_INLINE void doTextureCopy(u32 src, u32 dst, u32 size, u32 srcParam, u32 dstParam, u32 flags) {
    GxRegs* regs = getGxRegs();

    regs->ppf.in_addr = src >> 3;
    regs->ppf.out_addr = dst >> 3;
    regs->ppf.len = size;
    regs->ppf.tc_indim = srcParam;
    regs->ppf.tc_outdim = dstParam;
    regs->ppf.flags = flags;
    regs->ppf.cnt = PPF_EN;
}

KYGX_INLINE void doFlushCacheRegions(u32 addr0, u32 size0, u32 addr1, u32 size1, u32 addr2, u32 size2) {
    flushDCacheRange((void*)addr0, size0);

    if (size1) {
        flushDCacheRange((void*)addr1, size1);

        if (size2)
            flushDCacheRange((void*)addr2, size2);
    }
}

KYGX_INLINE void execCommand(const KYGXCmd* cmd) {
    switch (cmd->header & 0xFF) {
        case KYGX_CMDID_PROCESSCOMMANDLIST:
            doProcessCommandList(cmd->params[0], cmd->params[1], cmd->params[2] == 1, cmd->params[6] == 1);
            break;
        case KYGX_CMDID_MEMORYFILL:
            doMemoryFill(cmd->params[0], cmd->params[1], cmd->params[2], cmd->params[3], cmd->params[4], cmd->params[5], cmd->params[6]);
            break;
        case KYGX_CMDID_DISPLAYTRANSFER:
            doDisplayTransfer(cmd->params[0], cmd->params[1], cmd->params[2], cmd->params[3], cmd->params[4]);
            break;
        case KYGX_CMDID_TEXTURECOPY:
            doTextureCopy(cmd->params[0], cmd->params[1], cmd->params[2], cmd->params[3], cmd->params[4], cmd->params[5]);
            break;
        case KYGX_CMDID_FLUSHCACHEREGIONS:
            doFlushCacheRegions(cmd->params[0], cmd->params[1], cmd->params[2], cmd->params[3], cmd->params[4], cmd->params[5]);
            break;
    }
}