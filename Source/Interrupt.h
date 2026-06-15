/**
 * @file Interrupt.h
 * @brief Generic interface for a GX interrupt handler implementation.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_KYGX_INTERRUPT_H
#define GUARD_KYGX_INTERRUPT_H

#include <KYGX/GX.h>

// Pseudo interrupt.
#define KYGXIntr_Flush (KYGXIntr)(KYGXIntr_DMA + 1)

// Initialize interrupt handling.
void IntrInit(void);

// Terminate interrupt handling.
void IntrExit(void);

// Set used PSC units.
void IntrSetPSC(bool unit0, bool unit1);

// Signal flush pseudo interrupt.
void IntrSignalFlush(void);

// Function called internally, implemented by the runner.
void IntrCallback(KYGXIntr intrID);

#endif /* GUARD_KYGX_INTERRUPT_H */