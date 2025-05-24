#ifndef _BOILERPLATE_H
#define _BOILERPLATE_H

#include <GX/GX.h>

#ifdef CTRGX_BAREMETAL
#include <arm11/console.h>
#include <arm11/drivers/hid.h>
#endif // CTRGX_BAREMETAL

CTRGX_INLINE void* getTopFB(void) {
#ifdef CTRGX_BAREMETAL
    return GFX_getBuffer(GFX_LCD_TOP, GFX_SIDE_LEFT);
#else
    return gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
#endif
}

CTRGX_INLINE void graphicsInit(void) {
#ifdef CTRGX_BAREMETAL
    GFX_initDefault();
    consoleInit(GFX_LCD_BOT, NULL);
#else
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, NULL);
#endif // CTRGX_BAREMETAL
}

CTRGX_INLINE void graphicsExit(void) {
#ifdef CTRGX_BAREMETAL
    GFX_deinit();
#else
    gfxExit();
#endif // CTRGX_BAREMETAL
}

CTRGX_INLINE bool mainLoop(void) {
#ifdef CTRGX_BAREMETAL
    return true;
#else
    return aptMainLoop();
#endif // CTRGX_BAREMETAL
}

CTRGX_INLINE void swapBuffers(void) {
#ifdef CTRGX_BAREMETAL
    GFX_swapBuffers();
#else
    gfxSwapBuffers();
#endif // CTRGX_BAREMETAL
}

#endif /* _BOILERPLATE_H */