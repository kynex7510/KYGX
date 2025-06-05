#include <debug.h> // panicMsg
#include <kernel.h> // yieldTask

#include <GX/Defs.h>

void kygx_platform_yield(void) { yieldTask(); }
void kygx_platform_break(const char* msg) { panicMsg(msg); }