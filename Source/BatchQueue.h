/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_KYGX_BATCHQUEUE_H
#define GUARD_KYGX_BATCHQUEUE_H

#include <KYGX/GX.h>

typedef enum {
    BatchQueueError_Success,
    BatchQueueError_NoMemory,
    BatchQueueError_NoCommands,
} BatchQueueError;

typedef struct {
    KYGXCmd* cmdList;
    KYGXBatchCallback cb;
    void* cbData;
} CmdBatchInfo;

typedef struct {
    const KYGXCmd* cmdList;
    size_t queueCapacity;
    size_t index;
    size_t count;
} CmdIterator;

typedef struct {
    KYGXCmd* cmdList;
    size_t* sizeList;
    KYGXBatchCallback* cbList;
    void** cbDataList;
    size_t capacity;
    size_t index;
    size_t count;
} BatchQueue;

// Return NO_MEM if no mem.
BatchQueueError BatchQueueInit(BatchQueue* q, size_t capacity);
void BatchQueueDestroy(BatchQueue* q);

bool BatchQueueIsEmpty(BatchQueue* q);

// Return NO_CMDS if no commands, NO_MEM if queue is full.
BatchQueueError BatchQueuePush(BatchQueue* q, const KYGXCmd* commands, size_t numCommands, KYGXBatchCallback cb, void* cbData);

// Return NO_CMDS if queue empty.
KYGXError BatchQueuePop(BatchQueue* q, KYGXBatchCallback* cb, void** cbData);

// Return NO_CMDS if queue empty.
KYGXError CmdIteratorInit(CmdIterator* it, const BatchQueue* q);

// Return number of commands.
size_t CmdIteratorCount(CmdIterator* it);

// Return next command, or null if no more commands.
const KYGXCmd* CmdIteratorNext(CmdIterator* it);

#endif /* GUARD_KYGX_BATCHQUEUE_H */