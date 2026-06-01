/**
 * @file GX.h
 * @brief KYGX API.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_KYGX_H
#define GUARD_KYGX_H

#include <CTR/Defs.h>

#define KYGX_CMD_REQUESTDMA 0x00
#define KYGX_CMD_PROCESSCOMMANDLIST 0x01
#define KYGX_CMD_MEMORYFILL 0x02
#define KYGX_CMD_DISPLAYTRANSFER 0x03
#define KYGX_CMD_TEXTURECOPY 0x04
#define KYGX_CMD_FLUSHCACHEREGIONS 0x05

/// @brief Callback invoked on batch completion.
typedef void (*KYGXBatchCallback)(void* data);

/// @brief Error codes.
typedef enum {
    KYGXError_Success = 0,    ///< Success
    KYGXError_NoMemory = 1,   ///< No memory
    KYGXError_NoCommands = 2, ///< No commands
} KYGXError;

/// @brief GPU interrupts.
typedef enum {
    KYGXIntr_PDC0, ///< PICA Display Controller 0
    KYGXIntr_PDC1, ///< PICA Display Controller 1
    KYGXIntr_PSC,  ///< PICA Screen Clear
    KYGXIntr_PPF,  ///< PICA Pixel Format
    KYGXIntr_P3D,  ///< PICA 3D
    KYGXIntr_DMA,  ///< Corelink DMA

    KYGXIntr_VBlankTop = KYGXIntr_PDC0,
    KYGXIntr_VBlankBottom = KYGXIntr_PDC1,
    KYGXIntr_MemoryFill = KYGXIntr_PSC,
    KYGXIntr_DisplayTransfer = KYGXIntr_PPF,
    KYGXIntr_TextureCopy = KYGXIntr_PPF,
    KYGXIntr_ProcessCommandList = KYGXIntr_P3D,
    KYGXIntr_RequestDMA = KYGXIntr_DMA,
} KYGXIntr;

/// @brief GX Command.
typedef struct {
    uint32_t header;    ///< Header
    uint32_t params[7]; ///< Parameters
} KYGXCmd;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Initialize KYGX.
KYGXError kygxInit(size_t maxCommands);

// Finalize KYGX.
void kygxExit(void);

// Clear interrupt state.
void kygxClearIntr(KYGXIntr intrID);

// Wait until the specified interrupt has been triggered.
void kygxWaitIntr(KYGXIntr intrID);

// Push batch of commands. Return NO_CMDS if no cmds. Return NO_MEM if no_mem.
KYGXError kygxPushBatch(const KYGXCmd* commands, size_t numCommands, KYGXBatchCallback cb, void* cbData);

// Wait until all batches are processed.
void kygxWaitCompletion(void);

// Stop processing further batches.
void kygxSetHalt(bool halt, bool wait);

// Execute command synchronously.
void kygxExecSync(const KYGXCmd* command);

CTR_INLINE void kygxWaitVBlankTop(void) {
    kygxClearIntr(KYGXIntr_VBlankTop);
    kygxWaitIntr(KYGXIntr_VBlankTop);
}

CTR_INLINE void kygxWaitVBlankBottom(void) {
    kygxClearIntr(KYGXIntr_VBlankBottom);
    kygxWaitIntr(KYGXIntr_VBlankBottom);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* GUARD_KYGX_H */