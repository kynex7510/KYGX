/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CTR11/Allocator.h>
#include <CTR11/Assert.h>

#include "BatchQueue.h"

#include <string.h>

static inline void clearQueue(BatchQueue* q) {
    q->cmdList = NULL;
    q->sizeList = NULL;
    q->cbList = NULL;
    q->cbDataList = NULL;
    q->capacity = 0;
    q->index = 0;
    q->count = 0;
}

BatchQueueError BatchQueueInit(BatchQueue* q, size_t capacity) {
    CTR_ASSERT(q);

    clearQueue(q);

    // It is allowed to have a dummy queue, eg. when only sync commands are needed.
    if (!capacity)
        return BatchQueueError_Success;

    const size_t entryOverhead = sizeof(KYGXCmd) + sizeof(size_t) + sizeof(KYGXBatchCallback) + sizeof(void*);
    void* buffer = AllocMem(MemType_Virtual, entryOverhead * capacity);
    if (!buffer)
        return BatchQueueError_NoMemory;

    q->cmdList = (KYGXCmd*)buffer;
    q->sizeList = (size_t*)&q->cmdList[capacity];
    q->cbList = (KYGXBatchCallback*)&q->sizeList[capacity];
    q->cbDataList = (void**)&q->cbList[capacity];
    q->capacity = capacity;

    memset(q->sizeList, 0, sizeof(size_t) * capacity);
    return BatchQueueError_Success;
}

void BatchQueueDestroy(BatchQueue* q) {
    CTR_ASSERT(q);

    FreeMem(q->cmdList);
    clearQueue(q);
}

bool BatchQueueIsEmpty(BatchQueue* q) {
    CTR_ASSERT(q);
    return q->count == 0;
}

BatchQueueError BatchQueuePush(BatchQueue* q, const KYGXCmd* commands, size_t numCommands, KYGXBatchCallback cb, void* cbData) {
    CTR_ASSERT(q);
    CTR_ASSERT(commands);

    if (numCommands == 0)
        return BatchQueueError_NoCommands;

    if ((q->count + numCommands) > q->capacity)
        return BatchQueueError_NoMemory;

    const size_t firstIdx = (q->index + q->count) % q->capacity;

    for (size_t i = 0; i < numCommands; ++i)
        memcpy(&q->cmdList[(q->index + q->count + i) % q->capacity], &commands[i], sizeof(KYGXCmd));

    q->sizeList[firstIdx] = numCommands;
    q->cbList[firstIdx] = cb;
    q->cbDataList[firstIdx] = cbData;
    q->count += numCommands;

    return BatchQueueError_Success;
}

BatchQueueError BatchQueuePop(BatchQueue* q, KYGXBatchCallback* cb, void** cbData) {
    CTR_ASSERT(q);
    CTR_ASSERT(cb);
    CTR_ASSERT(cbData);

    if (q->count == 0)
        return BatchQueueError_NoCommands;

    const size_t idx = q->index % q->capacity;
    const size_t batchSize = q->sizeList[idx];

    // This must be the start of a batch.
    CTR_BREAK_IF(batchSize == 0);

    *cb = q->cbList[idx];
    *cbData = q->cbDataList[idx];

    q->sizeList[idx] = 0;
    q->index += batchSize;
    q->count -= batchSize;

    return BatchQueueError_Success;
}

BatchQueueError CmdIteratorInit(CmdIterator* it, const BatchQueue* q) {
    CTR_ASSERT(it);
    CTR_ASSERT(q);

    if (q->count == 0)
        return BatchQueueError_NoCommands;

    const size_t batchSize = q->sizeList[q->index % q->capacity];

    // This must be the start of a batch.
    CTR_BREAK_IF(batchSize == 0);

    it->cmdList = q->cmdList;
    it->queueCapacity = q->capacity;
    it->index = q->index;
    it->count = batchSize;
    return BatchQueueError_Success;
}

size_t CmdIteratorCount(CmdIterator* it) {
    CTR_ASSERT(it);
    return it->count;
}

const KYGXCmd* CmdIteratorNext(CmdIterator* it) {
    CTR_ASSERT(it);

    if (it->count == 0)
        return NULL;

    const size_t idx = it->index % it->queueCapacity;
    it->index += 1;
    it->count -= 1;

    return &it->cmdList[idx];
}