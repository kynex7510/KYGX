/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <3ds.h>

#include <CTR11/Memory.h>

#include <KYGX/Command/MemoryFill.h>
#include <KYGX/Command/DisplayTransfer.h>
#include <KYGX/Command/FlushCacheRegions.h>

#include <stdio.h>

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 400
#define SCREEN_BPP 3
#define FB_SIZE SCREEN_WIDTH * SCREEN_HEIGHT * SCREEN_BPP

static void* g_QTMRAMBuffer = NULL;
static u8 g_Red = 0xFF;
static u8 g_Green = 0xFF;
static u8 g_Blue = 0xFF;

static void clearScreen(void) {
    u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);

    // Prepare transfer.
    KYGXTransferBuffer transferSrc;
    transferSrc.addr = g_QTMRAMBuffer;
    transferSrc.width = SCREEN_WIDTH;
    transferSrc.height = SCREEN_HEIGHT;
    transferSrc.format = KYGXTransferFormat_RGB8;

    KYGXTransferBuffer transferDst;
    transferDst.addr = fb;
    transferDst.width = SCREEN_WIDTH;
    transferDst.height = SCREEN_HEIGHT;
    transferDst.format = KYGXTransferFormat_RGB8;

    KYGXTransferFlags transferFlags;
    transferFlags.mode = KYGXTransferMode_TiledToLinear;
    transferFlags.downscale = KYGXTransferDownscale_None;
    transferFlags.flip = KYGXTransferFlip_None;
    transferFlags.tileSize = KYGXTransferTileSize_8x8;

    // Clear buffer.
    u8* p = (u8*)g_QTMRAMBuffer;
    for (size_t i = 0; i < FB_SIZE; i += 3) {
        p[i] = g_Red;
        p[i + 1] = g_Green;
        p[i + 2] = g_Blue;
    }

    kygxSyncFlushSingleRegion(g_QTMRAMBuffer, FB_SIZE);
    kygxSyncDisplayTransfer(&transferSrc, &transferDst, &transferFlags);
}

int main(void) {
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, NULL);
    CTR_BREAK_IF(kygxInit(0) != KYGXError_Success);

    g_QTMRAMBuffer = AllocTypedMem(FB_SIZE, MemType_QTMRAM);
    if (!g_QTMRAMBuffer) {
        printf("QTMRAM buffer allocation failed\n");
        printf("NOTE: this test is for N3DS only\n");
        printf("Press START to exit\n");

        while (true) {
            hidScanInput();
            if (hidKeysDown() & KEY_START)
                break;

            gfxSwapBuffers();
            kygxWaitVBlankTop();
        }

        kygxExit();
        gfxExit();
        return 0;
    }

    bool updateConsole = true;
    while (true) {
        hidScanInput();
        const u32 kDown = hidKeysDown();

        if (kDown & KEY_START)
            break;

        if (updateConsole) {
            consoleClear();
            printf("RED: %u, GREEN: %u, BLUE: %u\n", g_Red, g_Green, g_Blue);
            printf("Press START to exit\n");
            updateConsole = false;
        }

        if (kDown & KEY_LEFT) {
            ++g_Red;
            updateConsole = true;
        } else if (kDown & KEY_UP) {
            ++g_Green;
            updateConsole = true;
        } else if (kDown & KEY_RIGHT) {
            ++g_Blue;
            updateConsole = true;
        } else if (kDown & KEY_DOWN) {
            g_Red = 0xFF;
            g_Green = 0xFF;
            g_Blue = 0xFF;
            updateConsole = true;
        }

        clearScreen();
        gfxSwapBuffers();
        kygxWaitVBlankTop();
    }

    FreeMem(g_QTMRAMBuffer);

    kygxExit();
    gfxExit();
    return 0;
}