/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _KYGX_HOS_STATEBEGIN_H
#define _KYGX_HOS_STATEBEGIN_H

#include <KYGX/GX.h>

typedef struct {
    LightLock lock;
    CondVar completionCV;
    CondVar haltCV;
    bool haltRequested;
    bool halted;
} PlatformState;

#endif /* _KYGX_HOS_STATEBEGIN_H */