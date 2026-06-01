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

/**
 * @brief Initialize KYGX.
 * This function can be called multiple times, but the number of max commands remains the same as the first call.
 * @param[in] maxCommands Capacity of the internal command queue. Pass 0 to allow synchronous commands only.
 * @return KYGXError_NoMemory if queue allocation failed.
 */
KYGXError kygxInit(size_t maxCommands);

/**
 * @brief Finalize KYGX.
 * This function must be called as many times as the number of times \ref kygxInit has been called.
 */
void kygxExit(void);

/**
 * @brief Clear interrupt state.
 * If an interrupt has been triggered prior to a \ref kygxWaitIntr call the function returns immediately. To avoid
 * this the interrupt must be cleared first.
 */
void kygxClearIntr(KYGXIntr intrID);

/**
 * @brief Wait until the specified interrupt has been triggered.
 */
void kygxWaitIntr(KYGXIntr intrID);

/**
 * @brief Push a batch of commands.
 * Commands are executed asynchronously in the order they appear in the batch. On batch completion the specified callback,
 * if any, is invoked.
 * @param[in] commands Array of commands.
 * @param[in] numCommands Number of commands.
 * @param[in] cb Batch callback, invoked on batch completion. Can be NULL.
 * @param[in] cbData Callback user data.
 * @return KYGXError_NoCommands if @p numCommands is 0; KYGXError_NoMemory if the command queue cannot contain this batch.
 */
KYGXError kygxPushBatch(const KYGXCmd* commands, size_t numCommands, KYGXBatchCallback cb, void* cbData);

/**
 * @brief Wait until all batches (and thus all queued commands) are executed.
 */
void kygxWaitCompletion(void);

/**
 * @brief Set halt status for the state machine.
 * If the state machine is halted further batches will not be processed. It's possible to request halting asynchronously:
 * @code
 * kygxSetHalt(true, false); // Request halt
 * // Run some code
 * kygxSetHalt(true, true); // Wait until execution has halted
 * @endcode
 * To restart command processing, clear the @p halt status.
 * @param[in] halt Whether to halt or not the state machine.
 * @param[in] wait If halting, wait until the current batch is completed.
 */
void kygxSetHalt(bool halt, bool wait);

/**
 * @brief Execute a command synchronously.
 * Batch execution will be paused for the whole duration of the operation.
 * @param[in] command Command to execute.
 */
void kygxExecSync(const KYGXCmd* command);

/**
 * @brief Wait for the top screen VBlank.
 */
CTR_INLINE void kygxWaitVBlankTop(void) {
    kygxClearIntr(KYGXIntr_VBlankTop);
    kygxWaitIntr(KYGXIntr_VBlankTop);
}

/**
 * @brief Wait for the bottom screen VBlank.
 */
CTR_INLINE void kygxWaitVBlankBottom(void) {
    kygxClearIntr(KYGXIntr_VBlankBottom);
    kygxWaitIntr(KYGXIntr_VBlankBottom);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* GUARD_KYGX_H */