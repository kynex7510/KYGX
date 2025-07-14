/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _KYGX_BM_STATEBEGIN_H
#define _KYGX_BM_STATEBEGIN_H

#include <KYGX/GX.h>

#include <kmutex.h>
#include <ksemaphore.h>

typedef struct {
    KHandle sema;
    u32 waiters;
} CV;

typedef struct {
    KHandle lock;
    CV completionCV;
    CV haltCV;
    bool haltRequested;
    bool halted;
} PlatformState;

KYGX_INLINE void CV_Init(CV* cv) {
    KYGX_ASSERT(cv);

    cv->sema = createSemaphore(0);
    cv->waiters = 0;
}

KYGX_INLINE void CV_Destroy(CV* cv) {
    KYGX_ASSERT(cv);
    deleteSemaphore(cv->sema);
}

KYGX_INLINE void CV_Wait(CV* cv, KHandle lock) {
    u32 w;
    do {
        w = __ldrex(&cv->waiters);
    } while (__strex(&cv->waiters, w + 1));

    KYGX_BREAK_UNLESS(unlockMutex(lock) == KRES_OK);
    KYGX_BREAK_UNLESS(waitForSemaphore(cv->sema) == KRES_OK);
    KYGX_BREAK_UNLESS(lockMutex(lock) == KRES_OK);
}

KYGX_INLINE void CV_Broadcast(CV* cv) {
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

#endif /* _KYGX_BM_STATEBEGIN_H */