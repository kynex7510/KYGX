/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <KYGX/GX.h>

#define CASE(x)   \
    case x:       \
        return #x 

#define DEFAULT_CASE() default: return "(unknown)"

const char* kygxErrorString(KYGXError error) {
    switch (error) {
        CASE(KYGX_ERROR_SUCCESS);
        CASE(KYGX_ERROR_SYSTEM);
        CASE(KYGX_ERROR_NO_MEM);
        CASE(KYGX_ERROR_BUSY);
        CASE(KYGX_ERROR_EMPTY);
        DEFAULT_CASE();
    }
}

const char* kygxIntrString(KYGXIntr intrID) {
    switch (intrID) {
        CASE(KYGX_INTR_PSC0);
        CASE(KYGX_INTR_PSC1);
        CASE(KYGX_INTR_PDC0);
        CASE(KYGX_INTR_PDC1);
        CASE(KYGX_INTR_PPF);
        CASE(KYGX_INTR_P3D);
        CASE(KYGX_INTR_DMA);
        DEFAULT_CASE();
    }
}