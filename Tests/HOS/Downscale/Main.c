/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <3ds.h>

#include <CTR11/Memory.h>

#include <KYGX/Command/FlushCacheRegions.h>
#include <KYGX/Command/DisplayTransfer.h>

#include <stdio.h>

int main(int argc, char* argv[]) {
    romfsInit();
    gfxInit(GSP_BGR8_OES, GSP_BGR8_OES, false);
    consoleInit(GFX_BOTTOM, NULL);
    CTR_BREAK_IF(kygxInit(0) != KYGXError_Success);

    // Load image.
    const size_t width = 480;
    const size_t height = 800;
    const size_t bpp = 24;
    const size_t imgSize = width * height * bpp >> 3;

    void* img = AllocMem(MemType_FCRAM, imgSize);
    CTR_BREAK_IF(!img);

    FILE* f = fopen("romfs:/EpicSkeleton.data", "rb");
    fread(img, imgSize, 1, f);
    fclose(f);

    kygxSyncFlushSingleRegion(img, imgSize);

    // Prepare transfer.
    KYGXTransferBuffer transferSrc;
    transferSrc.addr = img;
    transferSrc.width = width;
    transferSrc.height = height;
    transferSrc.format = KYGXTransferFormat_RGB8;

    KYGXTransferBuffer transferDst;
    transferDst.addr = NULL;
    transferDst.width = width;
    transferDst.height = height;
    transferDst.format = KYGXTransferFormat_RGB8;

    KYGXTransferFlags transferFlags;
    transferFlags.mode = KYGXTransferMode_TiledToLinear;
    transferFlags.downscale = KYGXTransferDownscale_2x2;
    transferFlags.flip = KYGXTransferFlip_None;
    transferFlags.tileSize = KYGXTransferTileSize_8x8;

    while (aptMainLoop()) {
        hidScanInput();
        const u32 kDown = hidKeysDown();
        if (kDown & KEY_START)
            break;

        transferDst.addr = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
        kygxSyncDisplayTransfer(&transferSrc, &transferDst, &transferFlags);
        gfxSwapBuffers();
        kygxWaitVBlankTop();
    }

    FreeMem(img);

    kygxExit();
    gfxExit();
    romfsExit();
    return 0;
}
