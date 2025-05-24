#include "Boilerplate.h"
#include "GX/Wrappers/MemoryFill.h"

#include <stdio.h>

#define FB_SIZE 240 * 400 * 3

static u8 g_Red = 0xFF;
static u8 g_Green = 0xFF;
static u8 g_Blue = 0xFF;

static void clearScreen() {
    GXMemoryFillBuffer fill;
    fill.addr = getTopFB();
    fill.size = FB_SIZE;
    fill.value = CTRGX_MEMORYFILL_VALUE_RGB8(g_Red, g_Green, g_Blue);
    fill.width = CTRGX_MEMORYFILL_WIDTH_24;
    ctrgxSyncMemoryFill(&fill, NULL);
}

int main(int argc, char* argv[]) {
    graphicsInit();
    ctrgxInit();

    bool updateConsole = true;
    while (mainLoop()) {
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
        swapBuffers();
        ctrgxWaitVBlank();
    }

    ctrgxExit();
    graphicsExit();
    return 0;
}
