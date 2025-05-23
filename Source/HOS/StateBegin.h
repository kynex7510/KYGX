#ifndef _CTRGX_HOS_STATEBEGIN_H
#define _CTRGX_HOS_STATEBEGIN_H

#include "GX/GX.h"

typedef struct {
    LightLock lock;
    CondVar completionCV;
    CondVar haltCV;
    bool haltRequested;
    bool halted;
} PlatformState;

#endif /* _CTRGX_HOS_STATEBEGIN_H */