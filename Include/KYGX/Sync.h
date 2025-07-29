/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _KYGX_SYNC_H
#define _KYGX_SYNC_H

#include <KYGX/Defs.h>

#ifdef KYGX_BAREMETAL

#include <kmutex.h>
#include <ksemaphore.h>

#include <stdint.h> // UINT32_MAX

typedef KHandle KYGXMtx;

typedef struct {
    KHandle sema;
    u32 waiters;
} KYGXCV;

KYGX_INLINE void kygxMtxInit(KYGXMtx* mtx) {
    KYGX_ASSERT(mtx);

    *mtx = createMutex();
    KYGX_ASSERT(*mtx);
}

KYGX_INLINE void kygxMtxDestroy(KYGXMtx* mtx) {
    KYGX_ASSERT(mtx);

    deleteMutex(*mtx);
    *mtx = 0;
}

KYGX_INLINE void kygxMtxAcquire(KYGXMtx* mtx) {
    KYGX_ASSERT(mtx);

    const KRes ret = lockMutex(*mtx);
    KYGX_ASSERT(ret == KRES_OK);
}

KYGX_INLINE void kygxMtxRelease(KYGXMtx* mtx) {
    KYGX_ASSERT(mtx);

    const KRes ret = unlockMutex(*mtx);
    KYGX_ASSERT(ret == KRES_OK);
}

KYGX_INLINE void kygxCVInit(KYGXCV* cv) {
    KYGX_ASSERT(cv);

    cv->sema = createSemaphore(0);
    cv->waiters = 0;
}

KYGX_INLINE void kygxCVDestroy(KYGXCV* cv) {
    KYGX_ASSERT(cv);
    deleteSemaphore(cv->sema);
}

KYGX_INLINE void kygxCVWait(KYGXCV* cv, KYGXMtx* mtx) {
    KYGX_ASSERT(cv);
    KYGX_ASSERT(mtx);

    u32 w;
    do {
        w = __ldrex(&cv->waiters);
    } while (__strex(&cv->waiters, w + 1));

    KYGX_ASSERT(unlockMutex(*mtx) == KRES_OK);
    KYGX_ASSERT(waitForSemaphore(cv->sema) == KRES_OK);
    KYGX_ASSERT(lockMutex(*mtx) == KRES_OK);
}

KYGX_INLINE void kygxCVSignal(KYGXCV* cv, u32 count) {
    KYGX_ASSERT(cv);

    u32 w;

    __dmb();
    do {
        w = __ldrex(&cv->waiters);
    } while (__strex(&cv->waiters, w > count ? w - count : 0));

    if (w) {
        // TODO: reschedule?
        signalSemaphore(cv->sema, w, false);
    } else {
        __dmb();
    }
}

KYGX_INLINE void kygxCVBroadcast(KYGXCV* cv) {
    KYGX_ASSERT(cv);
    kygxCVSignal(cv, UINT32_MAX);
}

#else

typedef LightLock KYGXMtx;
typedef CondVar KYGXCV;

KYGX_INLINE void kygxMtxInit(KYGXMtx* mtx) {
    KYGX_ASSERT(mtx);
    LightLock_Init(mtx);
}

KYGX_INLINE void kygxMtxDestroy(KYGXMtx* mtx) {
    KYGX_ASSERT(mtx);
    (void)mtx;
}

KYGX_INLINE void kygxMtxAcquire(KYGXMtx* mtx) {
    KYGX_ASSERT(mtx);
    LightLock_Lock(mtx);
}

KYGX_INLINE void kygxMtxRelease(KYGXMtx* mtx) {
    KYGX_ASSERT(mtx);
    LightLock_Unlock(mtx);
}

KYGX_INLINE void kygxCVInit(KYGXCV* cv) {
    KYGX_ASSERT(cv);
    CondVar_Init(cv);
}

KYGX_INLINE void kygxCVDestroy(KYGXCV* cv) {
    KYGX_ASSERT(cv);
    (void)cv;
}

KYGX_INLINE void kygxCVWait(KYGXCV* cv, KYGXMtx* mtx) {
    KYGX_ASSERT(cv);
    KYGX_ASSERT(mtx);
    CondVar_Wait(cv, mtx);
}

KYGX_INLINE void kygxCVSignal(KYGXCV* cv, u32 count) {
    KYGX_ASSERT(cv);
    CondVar_WakeUp(cv, count);
}

KYGX_INLINE void kygxCVBroadcast(KYGXCV* cv) {
    KYGX_ASSERT(cv);
    CondVar_Broadcast(cv);
}

#endif // KYGX_BAREMETAL

#endif /* _KYGX_SYNC_H */