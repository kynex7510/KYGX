/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <debug.h> // panicMsg
#include <kernel.h> // yieldTask
#include <drivers/cache.h> // invalidateDCacheRange

#include <KYGX/Defs.h>

void kygx_platform_yield(void) { yieldTask(); }
void kygx_platform_break(const char* msg) { panicMsg(msg); }
void kygxInvalidateDataCache(const void* addr, size_t size) { invalidateDCacheRange(addr, size); }
u32 kygxGetPhysicalAddress(const void* addr) { return (u32)addr; }
void* kygxGetVirtualAddress(u32 addr) { return (void*)addr; }