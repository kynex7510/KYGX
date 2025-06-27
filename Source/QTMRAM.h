#ifndef _KYGX_QTMRAM_H
#define _KYGX_QTMRAM_H

#ifdef KYGX_BAREMETAL
#include <mem_map.h>
#include <arm11/util/rbtree.h>
#else
#include <3ds/util/rbtree.h>
#endif // KYGX_BAREMETAL

#include <KYGX/Allocator.h>
#include <KYGX/Utility.h>

typedef struct {
    rbtree_node_t node;
    u32 base;
    size_t size;
} MemoryBlock;

static rbtree_t g_Tree;
static bool g_Initialized = false;
static u32 g_AllocBase = 0;
static u32 g_MaxAllocSize = 0;

static bool initRegion(void) {
#ifdef KYGX_BAREMETAL
    // TODO: mmu checks?
    // TODO: enable GPU access and stuff
    g_AllocBase = QTM_RAM_BASE;
    g_MaxAllocSize = QTM_RAM_SIZE;
    return true;
#else
    s64 out;

    g_AllocBase = 0;
    if (R_SUCCEEDED(svcGetProcessInfo(&out, CUR_PROCESS_HANDLE, 22)))
        g_AllocBase = out;

    g_MaxAllocSize = 0;
    if (R_SUCCEEDED(svcGetProcessInfo(&out, CUR_PROCESS_HANDLE, 23)))
        g_MaxAllocSize = out;

    return g_AllocBase && g_MaxAllocSize;
#endif // KYGX_BAREMETAL
}

static int blockComparator(const rbtree_node_t* lhs, const rbtree_node_t* rhs) {
    u32 a = ((MemoryBlock*)lhs)->base;
    u32 b = ((MemoryBlock*)rhs)->base;

    if (a < b)
        return -1;

    if (a > b)
        return 1;

    return 0;
}

static bool lazyInit(void) {
    if (!g_Initialized) {
        if (!initRegion())
            return false;

        rbtree_init(&g_Tree, blockComparator);
        g_Initialized = true;
    }

    return true;
}

static void* insertNode(u32 base, size_t size) {
    MemoryBlock* b = (MemoryBlock*)kygxAlloc(KYGX_MEM_HEAP, sizeof(MemoryBlock));
    if (b) {
        b->base = base;
        b->size = size;
        if (rbtree_insert(&g_Tree, &b->node));
        return (void*)b->base;
    }

    return NULL;
}

KYGX_INLINE void* qtmramMemAlign(size_t size, size_t alignment) {
    lazyInit();

    if (alignment < 8)
        alignment = 8;

    if (!kygxIsPo2(alignment))
        return NULL;

    // Get last memory block.
    MemoryBlock* last = (MemoryBlock*)rbtree_max(&g_Tree);
    if (!last) {
        // Insert if we have nothing.
        return insertNode(g_AllocBase, size);
    }

    // If there's space after the last block, use it.
    const u32 lastEndAligned = kygxAlignUp(last->base + last->size, alignment);
    if ((g_AllocBase + g_MaxAllocSize) - lastEndAligned >= size)
        return insertNode(lastEndAligned, size);

    // Look for space between existing memory blocks.
    MemoryBlock* current = last;
    MemoryBlock* prev = (MemoryBlock*)rbtree_node_prev(&current->node);

    while (prev) {
        const u32 prevEndAligned = kygxAlignUp(prev->base + prev->size, alignment);
        if (current->base - prevEndAligned >= size)
            return insertNode(prevEndAligned, size);

        current = prev;
        prev = (MemoryBlock*)rbtree_node_prev(&current->node);
    }

    // If there's space before the first block, use it.
    if (current->base - g_AllocBase >= size)
        return insertNode(g_AllocBase, size);

    return NULL;
}

KYGX_INLINE void* qtmramAlloc(size_t size) { return qtmramMemAlign(size, 0); }

KYGX_INLINE void qtmramFree(void* p) {
    lazyInit();

    MemoryBlock b;
    b.base = (u32)p;

    rbtree_node_t* found = rbtree_find(&g_Tree, &b.node);
    if (found) {
        rbtree_remove(&g_Tree, found, NULL);
        kygxFree(found);
    }
}

KYGX_INLINE size_t qtmramGetSize(const void* p) {
    lazyInit();

    MemoryBlock b;
    b.base = (u32)p;

    rbtree_node_t* found = rbtree_find(&g_Tree, &b.node);
    return found ? ((MemoryBlock*)found)->size : 0;
}

#endif // KYGX_QTMRAM_H