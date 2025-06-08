#include <GX/Allocator.h>
#include <GX/Wrappers/MemoryFill.h>
#include <GX/Wrappers/DisplayTransfer.h>
#include <GX/Wrappers/TextureCopy.h>

#include <stdio.h>

#define FB_SIZE 240 * 400 * 3
#define CMDBUFFER_CAPACITY 32

#define RECT_X 100
#define RECT_Y 80
#define RECT_WIDTH 200
#define RECT_HEIGHT 80

static GXCmdBuffer g_CmdBuffer;
static void* g_VRAMBuffer;

static void onCommandsCompleted(void* data) {
    (void)data;
    gfxScreenSwapBuffers(GFX_TOP, false);
}

static void clearScreen(void) {
    u16 screenWidth, screenHeight;
    u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &screenWidth, &screenHeight);

    // Prepare fill structure.
    GXMemoryFillBuffer fill;
    fill.addr = g_VRAMBuffer;
    fill.size = FB_SIZE;
    fill.value = KYGX_MEMORYFILL_VALUE_RGB8(0xFF, 0xFF, 0xFF);
    fill.width = KYGX_MEMORYFILL_WIDTH_24;

    // Prepare transfer flags.
    GXDisplayTransferFlags transferFlags;
    transferFlags.mode = KYGX_DISPLAYTRANSFER_MODE_T2L;
    transferFlags.srcFmt = KYGX_DISPLAYTRANSFER_FMT_RGB8;
    transferFlags.dstFmt = KYGX_DISPLAYTRANSFER_FMT_RGB8;
    transferFlags.downscale = KYGX_DISPLAYTRANSFER_DOWNSCALE_NONE;
    transferFlags.verticalFlip = false;
    transferFlags.blockMode32 = false;

    // Fill framebuffer with white through VRAM.
    kygxLock();
    kygxAddMemoryFill(&g_CmdBuffer, &fill, NULL);

    // Finalize: the same buffer should not be used with different commands at the same time.
    kygxCmdBufferFinalize(&g_CmdBuffer, NULL, NULL);

    kygxAddDisplayTransferChecked(&g_CmdBuffer, g_VRAMBuffer, fb, screenWidth, screenHeight, screenWidth, screenHeight, &transferFlags);
    kygxCmdBufferFinalize(&g_CmdBuffer, NULL, NULL);
    kygxUnlock(true);
}

static void drawRect(u16 x, u16 y, u16 width, u16 height) {
    // This is actually the screen height, as screens are rotated 90 degrees CW.
    u16 screenWidth;
    u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &screenWidth, NULL);

    // Prepare fill structure.
    GXMemoryFillBuffer fill;
    fill.addr = g_VRAMBuffer;
    fill.size = FB_SIZE;
    fill.value = KYGX_MEMORYFILL_VALUE_RGB8(0xFF, 0x00, 0x00);
    fill.width = KYGX_MEMORYFILL_WIDTH_24;

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
    kygxConvertTextureCopyRectRotated(&rect, screenWidth, KYGX_TEXTURECOPY_PIXEL_SIZE_RGB8, &offset, &size, &lineWidth, &gap);

    // Draw red rectangle through VRAM.
    kygxLock();
    kygxAddMemoryFill(&g_CmdBuffer, NULL, &fill);

    // Finalize: the same buffer should not be used with different commands at the same time.
    kygxCmdBufferFinalize(&g_CmdBuffer, NULL, NULL);

    kygxAddTextureCopy(&g_CmdBuffer, (u8*)g_VRAMBuffer + offset, fb + offset, size, lineWidth, gap, lineWidth, gap);
    kygxCmdBufferFinalize(&g_CmdBuffer, onCommandsCompleted, NULL);
    kygxUnlock(true);
}

int main(int argc, char* argv[]) {
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, NULL);
    kygxInit();

    g_VRAMBuffer = kygxAlloc(GX_MEM_VRAM, FB_SIZE);

    kygxCmdBufferAlloc(&g_CmdBuffer, CMDBUFFER_CAPACITY);
    kygxExchangeCmdBuffer(&g_CmdBuffer, true);

    printf("- Rect X: %u\n", RECT_X);
    printf("- Rect Y: %u\n", RECT_Y);
    printf("- Rect width: %u\n", RECT_WIDTH);
    printf("- Rect height: %u\n", RECT_HEIGHT);
    printf("Press START to exit\n");

    while (aptMainLoop()) {
        hidScanInput();
        const u32 kDown = hidKeysDown();

        if (kDown & KEY_START)
            break;

        clearScreen();
        drawRect(RECT_X, RECT_Y, RECT_WIDTH, RECT_HEIGHT);
        kygxWaitVBlank();
    }

    kygxExchangeCmdBuffer(NULL, true);
    kygxCmdBufferFree(&g_CmdBuffer);

    kygxFree(g_VRAMBuffer);

    kygxExit();
    gfxExit();
    return 0;
}