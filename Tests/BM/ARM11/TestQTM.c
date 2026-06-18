// https://gist.github.com/kynex7510/f760d71de575eae066c0795263703826

#include <CTR11/Allocator.h>
#include <KYGX/Command/MemoryFill.h>
#include <KYGX/Command/DisplayTransfer.h>
#include <KYGX/Command/FlushCacheRegions.h>

#include <arm11/fmt.h>
#include <arm11/power.h>
#include <arm11/console.h>
#include <arm11/drivers/hid.h>

#define FB_SIZE LCD_WIDTH_TOP * LCD_HEIGHT_TOP * 3

static void* g_QTMBuffer = NULL;
static u8 g_Red = 0xFF;
static u8 g_Green = 0xFF;
static u8 g_Blue = 0xFF;

static void clearScreen(void) {
    u8* fb = GFX_getBuffer(GFX_LCD_TOP, GFX_SIDE_LEFT);

    // Prepare transfer flags.
    KYGXDisplayTransferFlags transferFlags;
    transferFlags.mode = KYGX_DISPLAYTRANSFER_MODE_T2L;
    transferFlags.srcFmt = KYGX_DISPLAYTRANSFER_FMT_RGB8;
    transferFlags.dstFmt = KYGX_DISPLAYTRANSFER_FMT_RGB8;
    transferFlags.downscale = KYGX_DISPLAYTRANSFER_DOWNSCALE_NONE;
    transferFlags.verticalFlip = false;
    transferFlags.blockMode32 = false;

    // Clear buffer.
    for (size_t i = 0; i < FB_SIZE; i += 3) {
        u8* p = (u8*)g_QTMBuffer;
        p[i] = g_Red;
        p[i + 1] = g_Green;
        p[i + 2] = g_Blue;
    }

    kygxSyncFlushSingleRegion(g_QTMBuffer, FB_SIZE);
    kygxSyncDisplayTransferChecked(g_QTMBuffer, fb, LCD_WIDTH_TOP, LCD_HEIGHT_TOP, LCD_WIDTH_TOP, LCD_HEIGHT_TOP, &transferFlags);
}

int main(void) {
    GFX_init(GFX_BGR8, GFX_BGR565, GFX_TOP_2D);
    GFX_setLcdLuminance(80);
    consoleInit(GFX_LCD_BOT, NULL);
    CTR_BREAK_IF(kygxInit(0) != KYGXError_Success);

    g_QTMBuffer = AllocMem(MemType_QTMRAM, FB_SIZE);

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

    FreeMem(g_QTMBuffer);

    kygxExit();
    GFX_deinit();
    power_off();
    return 0;
}