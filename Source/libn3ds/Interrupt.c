/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <kernel.h>
#include <kevent.h>
#include <drivers/gfx.h>
#include <arm11/drivers/gx.h>
#include <arm11/drivers/gpu_regs.h>
#include <arm11/drivers/interrupt.h>

#include <CTR11/Assert.h>
#include <CTR11/Break.h>

#include "Interrupt.h"

#include <string.h>

/*
    Currently libn3ds provides no way of setting a callback for GPU interrupts (akin to gspSetEventCallback in libctru).
    The workaround is to overwrite each interrupt callback with our own. This breaks the gfx api, thus that can't be used
    until kygx is finalized.
*/

#define NUM_INTRS 7

#define INTR_PSC0 0
#define INTR_PSC1 1
#define INTR_PDC0 2
#define INTR_PDC1 3
#define INTR_PPF 4
#define INTR_P3D 5
#define INTR_FLUSH 6 // Emulated.

static KHandle g_AnyEvent;
static u8 g_IntrFlags[NUM_INTRS];

// Used to merge PSC interrupts together.
static bool g_HasPSC0 = false;
static bool g_HasPSC1 = false;

static bool g_UpdateGasAccMax = false;

static bool g_ExitThread = false;
static bool g_ExitedThread = false; // Join workaround.

// Calculate 1.0/x for half precision (1.5.10).
static inline u16 hprecip(u16 x) {
    const u32 exponent = (x >> 10) & 0x1F;
    const u32 mantissa = x & 0x03FF;

    s32 invExp = 15 - (s32)exponent;
    u32 divisor = (mantissa << 4) | 0x4000;

    u32 rem  = 0x4000;
    u32 quot = 0;
    u32 bit  = 0x4000;

    do {
        if (rem >= divisor) {
            rem -= divisor;
            quot |= bit;
        }

        divisor >>= 1;
        bit >>= 1;
    } while (bit);

    u32 sig = quot >> 4;

    while ((sig & 0x400) == 0) {
        sig <<= 1;
        invExp--;
    }

    s32 outExp = invExp + 15;

    if (outExp <= 0) {
        outExp = 0;
        sig = 0;
    } else if (outExp >= 31) {
        outExp = 31;
        sig = 0;
    }

    return ((u16)outExp << 10) | (sig & 0x3FF);
}

static inline void updateGasAccMax(void) {
    GxRegs* regs = getGxRegs();

    u16 feedback = regs->p3d[GPUREG_GAS_ACCMAX_FEEDBACK] >> 16;

    if (feedback && !(feedback & 0x8000))
        feedback = hprecip(feedback);        

    regs->p3d[GPUREG_GAS_ACCMAX] = feedback;
}

static inline void setIntr(size_t index, u8 v) {
    CTR_ASSERT(index < NUM_INTRS);

    u8* p = &g_IntrFlags[index];

    do {
        __ldrexb(p);
    } while (__strexb(p, v));
}

// Keep this as small as possible.
static void onInterrupt(u32 isr) {
    setIntr(isr - IRQ_PSC0, 1);
    signalEvent(g_AnyEvent, false);
}

static void intrHandler(void* unused) {
    while (true) {
        waitForEvent(g_AnyEvent);
        if (g_ExitThread)
            break;

        u8 intrFlags[NUM_INTRS];
        memcpy(intrFlags, g_IntrFlags, sizeof(g_IntrFlags));

        for (size_t i = 0; i < NUM_INTRS; ++i)
            setIntr(i, 0);

        if (CTR_LIKELY(intrFlags[INTR_PDC0]))
            IntrCallback(KYGXIntr_PDC0);

        if (CTR_LIKELY(intrFlags[INTR_PDC1]))
            IntrCallback(KYGXIntr_PDC1);

        if (intrFlags[INTR_PSC0]) {
            g_HasPSC0 = false;
            if (!g_HasPSC1)
                IntrCallback(KYGXIntr_PSC);
        }

        if (intrFlags[INTR_PSC1]) {
            g_HasPSC1 = false;
            if (!g_HasPSC0)
                IntrCallback(KYGXIntr_PSC);
        }

        if (intrFlags[INTR_PPF])
            IntrCallback(KYGXIntr_PPF);

        if (intrFlags[INTR_P3D]) {
            if (g_UpdateGasAccMax) {
                updateGasAccMax();
                g_UpdateGasAccMax = false;
            }

            IntrCallback(KYGXIntr_P3D);
        }

        if (intrFlags[INTR_FLUSH])
            IntrCallback(KYGXIntr_Flush);

        yieldTask();
    }

    //
    g_ExitedThread = true;
    //

    taskExit();
}

void IntrInit(void) {
    CTR_ASSERT(!g_AnyEvent);

    g_ExitThread = false;
    g_ExitedThread = false;

    memset(g_IntrFlags, 0, sizeof(g_IntrFlags));

    g_AnyEvent = createEvent(true);
    CTR_BREAK_IF(!g_AnyEvent);

    CTR_BREAK_IF(!createTask(0x100, 3, intrHandler, NULL));

    IRQ_registerIsr(IRQ_PSC0, 14, 0, onInterrupt);
    IRQ_registerIsr(IRQ_PSC1, 14, 0, onInterrupt);
    IRQ_registerIsr(IRQ_PDC0, 14, 0, onInterrupt);
    IRQ_registerIsr(IRQ_PDC1, 14, 0, onInterrupt);
    IRQ_registerIsr(IRQ_PPF, 14, 0, onInterrupt);
    IRQ_registerIsr(IRQ_P3D, 14, 0, onInterrupt);
}

void IntrExit(void) {
    CTR_ASSERT(g_AnyEvent);

    g_ExitThread = true;
    signalEvent(g_AnyEvent, true);

    IRQ_unregisterIsr(IRQ_PSC0);
    IRQ_unregisterIsr(IRQ_PSC1);
    IRQ_unregisterIsr(IRQ_PDC0);
    IRQ_unregisterIsr(IRQ_PDC1);
    IRQ_unregisterIsr(IRQ_PPF);
    IRQ_unregisterIsr(IRQ_P3D);

    while (!g_ExitedThread)
        yieldTask();

    deleteEvent(g_AnyEvent);

    // Attempt to fix gfx api.
    const GfxFmt topFmt = GFX_getFormat(GFX_LCD_TOP);
    const GfxFmt botFmt = GFX_getFormat(GFX_LCD_BOT);
    const GfxTopMode topMode = GFX_getTopMode();

    GFX_deinit();
    GFX_init(topFmt, botFmt, topMode);
}

void IntrSetPSC(bool unit0, bool unit1) {
    CTR_ASSERT(g_AnyEvent);

    g_HasPSC0 = unit0;
    g_HasPSC1 = unit1;
}

void IntrUpdateGasAccMax(bool update) {
    CTR_ASSERT(g_AnyEvent);

    g_UpdateGasAccMax = update;
}

void IntrSignalFlush(void) {
    CTR_ASSERT(g_AnyEvent);

    setIntr(INTR_FLUSH, 1);
    signalEvent(g_AnyEvent, false);
}