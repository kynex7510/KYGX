#include "GX/Wrappers/MemoryFill.h"
#include "GX/Wrappers/DisplayTransfer.h"
#include "GX/Wrappers/TextureCopy.h"

#include <arm11/fmt.h>
#include <arm11/power.h>
#include <arm11/console.h>
#include <arm11/allocator/vram.h>
#include <arm11/drivers/hid.h>

#define FB_SIZE LCD_WIDTH_TOP * LCD_HEIGHT_TOP * 3

#define CMDBUFFER_CAPACITY 32

#define RECT_X 100
#define RECT_Y 80
#define RECT_WIDTH 200
#define RECT_HEIGHT 80

static GXCmdBuffer g_CmdBuffer;
static void* g_VRAMBuffer;

static void onCommandsCompleted(void* data) {
    (void)data;
    GFX_swapBuffers();
}

static void clearScreen(void) {
    u8* fb = GFX_getBuffer(GFX_LCD_TOP, GFX_SIDE_LEFT);

    // Prepare fill structure.
    GXMemoryFillBuffer fill;
    fill.addr = g_VRAMBuffer;
    fill.size = FB_SIZE;
    fill.value = CTRGX_MEMORYFILL_VALUE_RGB8(0xFF, 0xFF, 0xFF);
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

    // Fill framebuffer with white through VRAM.
    ctrgxLock();
    ctrgxAddMemoryFill(&g_CmdBuffer, &fill, NULL);
    
    // Finalize: the same buffer should not be used with different commands at the same time.
    ctrgxCmdBufferFinalize(&g_CmdBuffer, NULL, NULL);

    ctrgxAddDisplayTransfer(&g_CmdBuffer, g_VRAMBuffer, fb, LCD_WIDTH_TOP, LCD_HEIGHT_TOP, LCD_WIDTH_TOP, LCD_HEIGHT_TOP, ctrgxMakeDisplayTransferFlags(&transferFlags));
    ctrgxCmdBufferFinalize(&g_CmdBuffer, NULL, NULL);
    ctrgxUnlock(true);
}

static void drawRect(u16 x, u16 y, u16 width, u16 height) {
    u8* fb = GFX_getBuffer(GFX_LCD_TOP, GFX_SIDE_LEFT);

    // Prepare fill structure.
    GXMemoryFillBuffer fill;
    fill.addr = g_VRAMBuffer;
    fill.size = FB_SIZE;
    fill.value = CTRGX_MEMORYFILL_VALUE_RGB8(0xFF, 0x00, 0x00);
    fill.width = CTRGX_MEMORYFILL_WIDTH_24;

    // Prepare rect params.
    GXTextureCopyRect rect;
    rect.x = x;
    rect.y = y;
    rect.width = width;
    rect.height = height;

    size_t offset = 0;
    size_t size = 0;
    u16 lineWidth = 0;
    u16 gap = 0;
    ctrgxConvertTextureCopyRectRotated(&rect, LCD_WIDTH_TOP, CTRGX_TEXTURECOPY_PIXEL_SIZE_RGB8, &offset, &size, &lineWidth, &gap);

    // Draw red rectangle through VRAM.
    ctrgxLock();
    ctrgxAddMemoryFill(&g_CmdBuffer, NULL, &fill);

    // Finalize: the same buffer should not be used with different commands at the same time.
    ctrgxCmdBufferFinalize(&g_CmdBuffer, NULL, NULL);

    ctrgxAddTextureCopy(&g_CmdBuffer, (u8*)g_VRAMBuffer + offset, fb + offset, size, lineWidth, gap, lineWidth, gap);
    ctrgxCmdBufferFinalize(&g_CmdBuffer, onCommandsCompleted, NULL);
    ctrgxUnlock(true);
}

int main(int argc, char* argv[]) {
    GFX_init(GFX_BGR8, GFX_BGR565, GFX_TOP_2D);
    GFX_setLcdLuminance(80);
    consoleInit(GFX_LCD_BOT, NULL);
    ctrgxInit();

    g_VRAMBuffer = vramAlloc(FB_SIZE);

    ctrgxCmdBufferAlloc(&g_CmdBuffer, CMDBUFFER_CAPACITY);
    ctrgxExchangeCmdBuffer(&g_CmdBuffer, true);

    ee_printf("- Rect X: %u\n", RECT_X);
    ee_printf("- Rect Y: %u\n", RECT_Y);
    ee_printf("- Rect width: %u\n", RECT_WIDTH);
    ee_printf("- Rect height: %u\n", RECT_HEIGHT);
    ee_printf("Press START to exit\n");

    while (true) {
        hidScanInput();
        const u32 kDown = hidKeysDown();

        if (kDown & KEY_START)
            break;

        clearScreen();
        drawRect(RECT_X, RECT_Y, RECT_WIDTH, RECT_HEIGHT);
        ctrgxWaitVBlank();
    }

    ctrgxExchangeCmdBuffer(NULL, true);
    ctrgxCmdBufferFree(&g_CmdBuffer);

    vramFree(g_VRAMBuffer);

    ctrgxExit();
    GFX_deinit();
    power_off();
    return 0;
}
