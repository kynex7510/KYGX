/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <KYGX/GX.h>

const char* kygxErrorString(KYGXError error) {
    switch (error) {
        case KYGXError_Success:
            return "Success";
        case KYGXError_System:
            return "System";
        case KYGXError_NoMem:
            return "No memory";
        case KYGXError_Busy:
            return "Busy";
        case KYGXError_Empty:
            return "Empty";
        default:
            return "(unknown)";
    }
}

const char* kygxIntrString(KYGXIntr intrID) {
    switch (intrID) {
        case KYGXIntr_PDC0:
            return "PICA Display Controller 0";
        case KYGXIntr_PDC1:
            return "PICA Display Controller 1";
        case KYGXIntr_PSC:
            return "PICA Screen Clear";
        case KYGXIntr_PPF:
            return "PICA Pixel Format";
        case KYGXIntr_P3D:
            return "PICA 3D";
        case KYGXIntr_DMA:
            return "Corelink DMA";
        default:
            return "(unknown)";
    }
}