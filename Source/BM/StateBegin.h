#ifndef _CTRGX_BM_STATEBEGIN_H
#define _CTRGX_BM_STATEBEGIN_H

#include <GX/GX.h>

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

CTRGX_INLINE void CV_Init(CV* cv) {
    CTRGX_ASSERT(cv);

    cv->sema = createSemaphore(0);
    cv->waiters = 0;
}

CTRGX_INLINE void CV_Destroy(CV* cv) {
    CTRGX_ASSERT(cv);
    deleteSemaphore(cv->sema);
}

CTRGX_INLINE void CV_Wait(CV* cv, KHandle lock) {
    u32 w;
    do {
        w = __ldrex(&cv->waiters);
    } while (__strex(&cv->waiters, w + 1));

    CTRGX_BREAK_UNLESS(unlockMutex(lock) == KRES_OK);
    CTRGX_BREAK_UNLESS(waitForSemaphore(cv->sema) == KRES_OK);
    CTRGX_BREAK_UNLESS(lockMutex(lock) == KRES_OK);
}

CTRGX_INLINE void CV_Broadcast(CV* cv) {
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

#endif /* _CTRGX_BM_STATEBEGIN_H */