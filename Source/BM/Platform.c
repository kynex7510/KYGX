#include <debug.h> // panicMsg
#include <kernel.h> // yieldTask

#include <GX/Defs.h>

void ctrgx_platform_yield(void) { yieldTask(); }
void ctrgx_platform_break(const char* msg) { panicMsg(msg); }