#include <KYGX/Wrappers/FlushCacheRegions.h>
#include <KYGX/Wrappers/DisplayTransfer.h>

#include <stdio.h>

int main(int argc, char* argv[]) {
    romfsInit();
    gfxInit(GSP_BGR8_OES, GSP_BGR8_OES, false);
    consoleInit(GFX_BOTTOM, NULL);
    kygxInit();

    // Load image.
    const size_t width = 480;
    const size_t height = 800;
    const size_t bpp = 24;
    const size_t imgSize = width * height * bpp >> 3;

    void* img = kygxAlloc(KYGX_MEM_LINEAR, imgSize);
    if (!img)
        svcBreak(USERBREAK_PANIC);

    FILE* f = fopen("romfs:/EpicSkeleton.data", "rb");
    fread(img, imgSize, 1, f);
    fclose(f);

    kygxSyncFlushSingleBuffer(img, imgSize);

    // Prepare transfer flags.
    KYGXDisplayTransferFlags transferFlags;
    transferFlags.mode = KYGX_DISPLAYTRANSFER_MODE_T2L;
    transferFlags.srcFmt = KYGX_DISPLAYTRANSFER_FMT_RGB8;
    transferFlags.dstFmt = KYGX_DISPLAYTRANSFER_FMT_RGB8;
    transferFlags.downscale = KYGX_DISPLAYTRANSFER_DOWNSCALE_2X2;
    transferFlags.verticalFlip = false;
    transferFlags.blockMode32 = false;

    while (aptMainLoop()) {
        hidScanInput();
        const u32 kDown = hidKeysDown();
        if (kDown & KEY_START)
            break;

        u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
        kygxSyncDisplayTransferChecked(img, fb, width, height, width, height, &transferFlags);
        gfxSwapBuffers();
        kygxWaitVBlank();
    }

    kygxFree(img);

    kygxExit();
    gfxExit();
    romfsExit();
    return 0;
}
