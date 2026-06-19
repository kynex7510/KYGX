#include <CTR11/Allocator.h>
#include <KYGX/Command/MemoryFill.h>
#include <KYGX/Command/DisplayTransfer.h>

#include <arm11/fmt.h>
#include <arm11/power.h>
#include <arm11/console.h>
#include <arm11/drivers/hid.h>

#define FB_SIZE LCD_WIDTH_TOP * LCD_HEIGHT_TOP * 3

static void* g_VRAMBuffer = NULL;
static u8 g_Red = 0xFF;
static u8 g_Green = 0xFF;
static u8 g_Blue = 0xFF;

static void clearScreen(void) {
    // Prepare fill structure.
    KYGXFill fill;
    fill.addr = g_VRAMBuffer;
    fill.size = FB_SIZE;
    fill.value = KYGX_RGB8_PIXEL(g_Red, g_Green, g_Blue);
    fill.width = KYGXFillWidth_RGB8;

    // Prepare transfer.
    KYGXTransferBuffer transferSrc;
    transferSrc.addr = g_VRAMBuffer;
    transferSrc.width = LCD_WIDTH_TOP;
    transferSrc.height = LCD_HEIGHT_TOP;
    transferSrc.format = KYGXTransferFormat_RGB8;

    KYGXTransferBuffer transferDst;
    transferDst.addr = GFX_getBuffer(GFX_LCD_TOP, GFX_SIDE_LEFT);
    transferDst.width = LCD_WIDTH_TOP;
    transferDst.height = LCD_HEIGHT_TOP;
    transferDst.format = KYGXTransferFormat_RGB8;
    
    KYGXTransferFlags transferFlags;
    transferFlags.mode = KYGXTransferMode_TiledToLinear;
    transferFlags.downscale = KYGXTransferDownscale_None;
    transferFlags.flip = KYGXTransferFlip_None;
    transferFlags.tileSize = KYGXTransferTileSize_8x8;

    // Fill framebuffer through VRAM.
    kygxSyncMemoryFill(&fill, NULL);
    kygxSyncDisplayTransfer(&transferSrc, &transferDst, &transferFlags);
}

int main(void) {
    GFX_init(GFX_BGR8, GFX_BGR565, GFX_TOP_2D);
    GFX_setLcdLuminance(80);
    consoleInit(GFX_LCD_BOT, NULL);
    CTR_BREAK_IF(kygxInit(0) != KYGXError_Success);

    g_VRAMBuffer = AllocMem(MemType_VRAM, FB_SIZE);

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
        kygxWaitVBlankTop();
    }

    FreeMem(g_VRAMBuffer);

    kygxExit();
    GFX_deinit();
    power_off();
    return 0;
}