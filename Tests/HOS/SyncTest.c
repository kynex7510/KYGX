#include <GX/Wrappers/MemoryFill.h>
#include <GX/Wrappers/DisplayTransfer.h>

#include <stdio.h>

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 400
#define SCREEN_BPP 3
#define FB_SIZE SCREEN_WIDTH * SCREEN_HEIGHT * SCREEN_BPP

static void* g_VRAMBuffer = NULL;
static u8 g_Red = 0xFF;
static u8 g_Green = 0xFF;
static u8 g_Blue = 0xFF;

static void clearScreen(void) {
    u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);

    // Prepare fill structure.
    GXMemoryFillBuffer fill;
    fill.addr = g_VRAMBuffer;
    fill.size = FB_SIZE;
    fill.value = CTRGX_MEMORYFILL_VALUE_RGB8(g_Red, g_Green, g_Blue);
    fill.width = CTRGX_MEMORYFILL_WIDTH_24;

    // Prepare transfer flags.
    GXDisplayTransferFlags transferFlags;
    transferFlags.srcFmt = CTRGX_DISPLAYTRANSFER_FMT_RGB8;
    transferFlags.dstFmt = CTRGX_DISPLAYTRANSFER_FMT_RGB8;
    transferFlags.downscale = CTRGX_DISPLAYTRANSFER_DOWNSCALE_NONE;
    transferFlags.verticalFlip = false;
    transferFlags.makeTiled = false;
    transferFlags.dontMakeLinear = false;
    transferFlags.blockMode32 = false;

    // Fill framebuffer through VRAM.
    ctrgxSyncMemoryFill(&fill, NULL);
    ctrgxSyncDisplayTransfer(g_VRAMBuffer, fb, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT, ctrgxMakeDisplayTransferFlags(&transferFlags));
}

int main(int argc, char* argv[]) {
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, NULL);
    ctrgxInit();

    g_VRAMBuffer = vramAlloc(FB_SIZE);

    bool updateConsole = true;
    while (aptMainLoop()) {
        hidScanInput();
        const u32 kDown = hidKeysDown();

        if (kDown & KEY_START)
            break;

        if (updateConsole) {
            consoleClear();
            printf("RED: %u, GREEN: %u, BLUE: %u\n", g_Red, g_Green, g_Blue);
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
        ctrgxWaitVBlank();
    }

    vramFree(g_VRAMBuffer);

    ctrgxExit();
    gfxExit();
    return 0;
}
