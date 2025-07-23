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

typedef KHandle KYGXLock;

typedef struct {
    KHandle sema;
    u32 waiters;
} KYGXCV;

KYGX_INLINE void kygxLockInit(KYGXLock* lock) {
    KYGX_ASSERT(lock);

    *lock = createMutex();
    KYGX_ASSERT(*lock);
}

KYGX_INLINE void kygxLockDestroy(KYGXLock* lock) {
    KYGX_ASSERT(lock);

    deleteMutex(*lock);
    *lock = 0;
}

KYGX_INLINE void kygxLockAcquire(KYGXLock* lock) {
    KYGX_ASSERT(lock);

    const KRes ret = lockMutex(*lock);
    KYGX_ASSERT(ret == KRES_OK);
}

KYGX_INLINE void kygxLockRelease(KYGXLock* lock) {
    KYGX_ASSERT(lock);

    const KRes ret = unlockMutex(*lock);
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

KYGX_INLINE void kygxCVWait(KYGXCV* cv, KYGXLock* lock) {
    KYGX_ASSERT(cv);
    KYGX_ASSERT(lock);

    u32 w;
    do {
        w = __ldrex(&cv->waiters);
    } while (__strex(&cv->waiters, w + 1));

    KYGX_BREAK_UNLESS(unlockMutex(*lock) == KRES_OK);
    KYGX_BREAK_UNLESS(waitForSemaphore(cv->sema) == KRES_OK);
    KYGX_BREAK_UNLESS(lockMutex(*lock) == KRES_OK);
}

KYGX_INLINE void kygxCVBroadcast(KYGXCV* cv) {
    KYGX_ASSERT(cv);

    u32 w;

    __dmb();
    do {
        w = __ldrex(&cv->waiters);
    } while (__strex(&cv->waiters, 0));

    if (w) {
        // TODO: reschedule?
        signalSemaphore(cv->sema, w, false);
    } else {
        __dmb();
    }
}

#else

typedef LightLock KYGXLock;
typedef CondVar KYGXCV;

KYGX_INLINE void kygxLockInit(KYGXLock* lock) {
    KYGX_ASSERT(lock);
    LightLock_Init(lock);
}

KYGX_INLINE void kygxLockDestroy(KYGXLock* lock) {
    KYGX_ASSERT(lock);
    (void)lock;
}

KYGX_INLINE void kygxLockAcquire(KYGXLock* lock) {
    KYGX_ASSERT(lock);
    LightLock_Lock(lock);
}

KYGX_INLINE void kygxLockRelease(KYGXLock* lock) {
    KYGX_ASSERT(lock);
    LightLock_Unlock(lock);
}

KYGX_INLINE void kygxCVInit(KYGXCV* cv) {
    KYGX_ASSERT(cv);
    CondVar_Init(cv);
}

KYGX_INLINE void kygxCVDestroy(KYGXCV* cv) {
    KYGX_ASSERT(cv);
    (void)cv;
}

KYGX_INLINE void kygxCVWait(KYGXCV* cv, KYGXLock* lock) {
    KYGX_ASSERT(cv);
    KYGX_ASSERT(lock);
    CondVar_Wait(cv, lock);
}

KYGX_INLINE void kygxCVBroadcast(KYGXCV* cv) {
    KYGX_ASSERT(cv);
    CondVar_Broadcast(cv);
}

#endif // KYGX_BAREMETAL

#endif /* _KYGX_SYNC_H */