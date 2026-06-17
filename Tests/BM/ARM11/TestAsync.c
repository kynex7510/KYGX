#include <CTR11/Allocator.h>
#include <KYGX/Command/MemoryFill.h>
#include <KYGX/Command/DisplayTransfer.h>
#include <KYGX/Command/TextureCopy.h>

#include <arm11/fmt.h>
#include <arm11/power.h>
#include <arm11/console.h>
#include <arm11/drivers/hid.h>

#define CMDBUFFER_CAPACITY 32

#define SCREEN_ROTATED 1

#if SCREEN_ROTATED

#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT 240

#define RECT_X 100
#define RECT_Y 80
#define RECT_WIDTH 200
#define RECT_HEIGHT 80

#else

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 400

#define RECT_X 80
#define RECT_Y 100
#define RECT_WIDTH 80
#define RECT_HEIGHT 200

#endif // SCREEN_ROTATED

#define SCREEN_PIXEL_SIZE 3
#define FB_SIZE SCREEN_WIDTH * SCREEN_HEIGHT * SCREEN_PIXEL_SIZE

static void* g_VRAMBuffer;

static void onCommandsCompleted(void* data) {
    (void)data;
    GFX_swapBuffers();
}

static void clearScreen(void) {
    u8* fb = GFX_getBuffer(GFX_LCD_TOP, GFX_SIDE_LEFT);

    // Prepare fill structure.
    KYGXMemoryFillBuffer fill;
    fill.addr = g_VRAMBuffer;
    fill.size = FB_SIZE;
    fill.value = KYGX_MEMORYFILL_VALUE_RGB8(0xFF, 0xFF, 0xFF);
    fill.width = KYGX_MEMORYFILL_WIDTH_24;

    // Prepare transfer flags.
    KYGXDisplayTransferFlags transferFlags;
    transferFlags.mode = KYGX_DISPLAYTRANSFER_MODE_T2L;
    transferFlags.srcFmt = KYGX_DISPLAYTRANSFER_FMT_RGB8;
    transferFlags.dstFmt = KYGX_DISPLAYTRANSFER_FMT_RGB8;
    transferFlags.downscale = KYGX_DISPLAYTRANSFER_DOWNSCALE_NONE;
    transferFlags.verticalFlip = false;
    transferFlags.blockMode32 = false;

    // Fill framebuffer with white through VRAM.
    KYGXCmd tmp;
    kygxMakeMemoryFill(&tmp, &fill, NULL);
    
    // Split commands, as the same buffer should not be used with different commands at the same time.
    kygxPushBatch(&tmp, 1, NULL, NULL);

    kygxMakeDisplayTransferChecked(&tmp, g_VRAMBuffer, fb, LCD_WIDTH_TOP, LCD_HEIGHT_TOP, LCD_WIDTH_TOP, LCD_HEIGHT_TOP, &transferFlags);
    kygxPushBatch(&tmp, 1, NULL, NULL);
}

static void drawRect(u16 x, u16 y, u16 width, u16 height) {
    u8* fb = GFX_getBuffer(GFX_LCD_TOP, GFX_SIDE_LEFT);

    // Prepare fill structure.
    KYGXMemoryFillBuffer fill;
    fill.addr = g_VRAMBuffer;
    fill.size = FB_SIZE;
    fill.value = KYGX_MEMORYFILL_VALUE_RGB8(0xFF, 0x00, 0x00);
    fill.width = KYGX_MEMORYFILL_WIDTH_24;

    // Prepare rect params.
    KYGXTextureCopySurface srcSurface;
    srcSurface.addr = g_VRAMBuffer;
    srcSurface.width = SCREEN_WIDTH;
    srcSurface.height = SCREEN_HEIGHT;
    srcSurface.pixelSize = SCREEN_PIXEL_SIZE;
    srcSurface.rotated = SCREEN_ROTATED;

    KYGXTextureCopySurface dstSurface;
    dstSurface.addr = fb;
    dstSurface.width = SCREEN_WIDTH;
    dstSurface.height = SCREEN_HEIGHT;
    dstSurface.pixelSize = SCREEN_PIXEL_SIZE;
    dstSurface.rotated = SCREEN_ROTATED;

    KYGXTextureCopyRect rect;
    rect.x = RECT_X;
    rect.y = RECT_Y;
    rect.width = RECT_WIDTH;
    rect.height = RECT_HEIGHT;

    // Draw red rectangle through VRAM.
    KYGXCmd tmp;
    kygxMakeMemoryFill(&tmp, NULL, &fill);

    // Split commands, as the same buffer should not be used with different commands at the same time.
    kygxPushBatch(&tmp, 1, NULL, NULL);

    kygxMakeRectCopy(&tmp, &srcSurface, &rect, &dstSurface, &rect);
    kygxPushBatch(&tmp, 1, onCommandsCompleted, NULL);
}

int main(int argc, char* argv[]) {
    GFX_init(GFX_BGR8, GFX_BGR565, GFX_TOP_2D);
    GFX_setLcdLuminance(80);
    consoleInit(GFX_LCD_BOT, NULL);
    CTR_BREAK_IF(kygxInit(8) != KYGXError_Success);

    g_VRAMBuffer = AllocMem(MemType_VRAM, FB_SIZE);

    ee_printf("- Rect X: %u\n", RECT_X);
    ee_printf("- Rect Y: %u\n", RECT_Y);
    ee_printf("- Rect width: %u\n", RECT_WIDTH);
    ee_printf("- Rect height: %u\n", RECT_HEIGHT);
    ee_printf("- Is rotated? %s\n", (SCREEN_ROTATED ? "Yes" : "No"));
    ee_printf("Press START to exit\n");

    while (true) {
        hidScanInput();
        const u32 kDown = hidKeysDown();

        if (kDown & KEY_START)
            break;

        clearScreen();
        drawRect(RECT_X, RECT_Y, RECT_WIDTH, RECT_HEIGHT);
        kygxWaitVBlankTop();
    }

    FreeMem(g_VRAMBuffer);

    kygxExit();
    GFX_deinit();
    power_off();
    return 0;
}
