#include <GX/Allocator.h>
#include <GX/Wrappers/MemoryFill.h>
#include <GX/Wrappers/DisplayTransfer.h>

#include <arm11/fmt.h>
#include <arm11/power.h>
#include <arm11/console.h>
#include <arm11/allocator/vram.h>
#include <arm11/drivers/hid.h>

#define FB_SIZE LCD_WIDTH_TOP * LCD_HEIGHT_TOP * 3

static void* g_VRAMBuffer = NULL;
static u8 g_Red = 0xFF;
static u8 g_Green = 0xFF;
static u8 g_Blue = 0xFF;

static void clearScreen(void) {
    u8* fb = GFX_getBuffer(GFX_LCD_TOP, GFX_SIDE_LEFT);

    // Prepare fill structure.
    GXMemoryFillBuffer fill;
    fill.addr = g_VRAMBuffer;
    fill.size = FB_SIZE;
    fill.value = CTRGX_MEMORYFILL_VALUE_RGB8(g_Red, g_Green, g_Blue);
    fill.width = CTRGX_MEMORYFILL_WIDTH_24;

    // Prepare transfer flags.
    GXDisplayTransferFlags transferFlags;
    transferFlags.mode = CTRGX_DISPLAYTRANSFER_MODE_T2L;
    transferFlags.srcFmt = CTRGX_DISPLAYTRANSFER_FMT_RGB8;
    transferFlags.dstFmt = CTRGX_DISPLAYTRANSFER_FMT_RGB8;
    transferFlags.downscale = CTRGX_DISPLAYTRANSFER_DOWNSCALE_NONE;
    transferFlags.verticalFlip = false;
    transferFlags.blockMode32 = false;

    // Fill framebuffer through VRAM.
    ctrgxSyncMemoryFill(&fill, NULL);
    ctrgxSyncDisplayTransfer(g_VRAMBuffer, fb, LCD_WIDTH_TOP, LCD_HEIGHT_TOP, LCD_WIDTH_TOP, LCD_HEIGHT_TOP, ctrgxMakeDisplayTransferFlags(&transferFlags));
}

int main(void) {
    GFX_init(GFX_BGR8, GFX_BGR565, GFX_TOP_2D);
    GFX_setLcdLuminance(80);
    consoleInit(GFX_LCD_BOT, NULL);
    ctrgxInit();

    g_VRAMBuffer = ctrgxAlloc(GX_MEM_VRAM, FB_SIZE);

    bool updateConsole = true;
    while (true) {
        hidScanInput();
        const u32 kDown = hidKeysDown();

        if (kDown & KEY_START)
            break;

        if (updateConsole) {
            consoleClear();
            ee_printf("RED: %u, GREEN: %u, BLUE: %u\n", g_Red, g_Green, g_Blue);
            ee_printf("Press START to exit\n");
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
        GFX_swapBuffers();
        ctrgxWaitVBlank();
    }

    ctrgxFree(g_VRAMBuffer);

    ctrgxExit();
    GFX_deinit();
    power_off();
    return 0;
}