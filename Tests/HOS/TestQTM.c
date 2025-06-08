#include <GX/Allocator.h>
#include <GX/Wrappers/MemoryFill.h>
#include <GX/Wrappers/DisplayTransfer.h>
#include <GX/Wrappers/FlushCacheRegions.h>

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

    // Prepare flush buffer.
    GXFlushCacheRegionsBuffer flush;
    flush.addr = g_QTMRAMBuffer;
    flush.size = FB_SIZE;

    // Prepare transfer flags.
    GXDisplayTransferFlags transferFlags;
    transferFlags.mode = KYGX_DISPLAYTRANSFER_MODE_T2L;
    transferFlags.srcFmt = KYGX_DISPLAYTRANSFER_FMT_RGB8;
    transferFlags.dstFmt = KYGX_DISPLAYTRANSFER_FMT_RGB8;
    transferFlags.downscale = KYGX_DISPLAYTRANSFER_DOWNSCALE_NONE;
    transferFlags.verticalFlip = false;
    transferFlags.blockMode32 = false;

    // Clear buffer.
    u8* p = (u8*)g_QTMRAMBuffer;
    for (size_t i = 0; i < FB_SIZE; i += 3) {
        p[i] = g_Red;
        p[i + 1] = g_Green;
        p[i + 2] = g_Blue;
    }

    kygxSyncFlushCacheRegions(&flush, NULL, NULL);
    kygxSyncDisplayTransferChecked(g_QTMRAMBuffer, fb, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT, &transferFlags);
}

int main(void) {
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, NULL);
    kygxInit();

    g_QTMRAMBuffer = kygxAlloc(GX_MEM_QTMRAM, FB_SIZE);
    if (!g_QTMRAMBuffer) {
        printf("QTMRAM buffer allocation failed\n");
        printf("NOTE: this test is for N3DS only\n");
        printf("Press START to exit\n");

        while (true) {
            hidScanInput();
            if (hidKeysDown() & KEY_START)
                break;

            gfxSwapBuffers();
            kygxWaitVBlank();
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
        gfxSwapBuffers();;
        kygxWaitVBlank();
    }

    kygxFree(g_QTMRAMBuffer);

    kygxExit();
    gfxExit();
    return 0;
}