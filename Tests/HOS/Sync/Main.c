#include <3ds.h>

#include <CTR11/Allocator.h>

#include <KYGX/Command/MemoryFill.h>
#include <KYGX/Command/DisplayTransfer.h>

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
    KYGXFill fill;
    fill.addr = g_VRAMBuffer;
    fill.size = FB_SIZE;
    fill.value = KYGX_RGB8_PIXEL(g_Red, g_Green, g_Blue);
    fill.width = KYGXFillWidth_RGB8;

    // Prepare transfer.
    KYGXTransferSurface transferSrc;
    transferSrc.addr = g_VRAMBuffer;
    transferSrc.width = SCREEN_WIDTH;
    transferSrc.height = SCREEN_HEIGHT;
    transferSrc.format = KYGXTransferFormat_RGB8;

    KYGXTransferSurface transferDst;
    transferDst.addr = fb;
    transferDst.width = SCREEN_WIDTH;
    transferDst.height = SCREEN_HEIGHT;
    transferDst.format = KYGXTransferFormat_RGB8;

    KYGXTransferFlags transferFlags;
    transferFlags.mode = KYGXTransferMode_TiledToLinear;
    transferFlags.downscale = KYGXTransferDownscale_None;
    transferFlags.flip = KYGXTransferFlip_None;
    transferFlags.blockMode = KYGXTransferBlockMode_8;

    // Fill framebuffer through VRAM.
    kygxSyncMemoryFill(&fill, NULL);
    kygxSyncDisplayTransfer(&transferSrc, &transferDst, &transferFlags);
}

int main(int argc, char* argv[]) {
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, NULL);
    CTR_BREAK_IF(kygxInit(0) != KYGXError_Success);

    g_VRAMBuffer = AllocMem(MemType_VRAM, FB_SIZE);

    bool updateConsole = true;
    while (aptMainLoop()) {
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

    FreeMem(g_VRAMBuffer);

    kygxExit();
    gfxExit();
    return 0;
}
