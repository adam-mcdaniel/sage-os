#include <kmalloc.h>
#include <stdint.h>
#include <util.h>
#include <mmu.h>
#include <page.h>
#include <debug.h>

// The heap virtual address will NOT map 1-to-1 with physical addresses.

typedef struct Block {
    void *addr;
    struct Block *next;
    size_t size;
} Block;

typedef struct {
    Block *free;   // first free block
    Block *used;   // first used block
    Block *fresh;  // first available blank block
    size_t top;    // top free addr
} Heap;

static Heap *heap             = NULL;
static const void *heap_limit = NULL;
static size_t heap_split_thresh;
static size_t heap_alignment;
static size_t heap_max_blocks;

/**
 * If compaction is enabled, inserts block
 * into free list, sorted by addr.
 * If disabled, add block has new head of
 * the free list.
 */
static void insert_block(Block *block)
{
    Block *ptr  = heap->free;
    Block *prev = NULL;
    while (ptr != NULL) {
        if ((size_t)block->addr <= (size_t)ptr->addr) {
            break;
        }
        prev = ptr;
        ptr  = ptr->next;
    }
    if (prev != NULL) {
        prev->next = block;
    }
    else {
        heap->free = block;
    }
    block->next = ptr;
}

#ifndef TA_DISABLE_COMPACT
static void release_blocks(Block *scan, Block *to)
{
    Block *scan_next;
    while (scan != to) {
        scan_next   = scan->next;
        scan->next  = heap->fresh;
        heap->fresh = scan;
        scan->addr  = 0;
        scan->size  = 0;
        scan        = scan_next;
    }
}

static void compact()
{
    Block *ptr = heap->free;
    Block *prev;
    Block *scan;
    while (ptr != NULL) {
        prev = ptr;
        scan = ptr->next;
        while (scan != NULL && (size_t)prev->addr + prev->size == (size_t)scan->addr) {
            prev = scan;
            scan = scan->next;
        }
        if (prev != ptr) {
            size_t new_size = (size_t)prev->addr - (size_t)ptr->addr + prev->size;
            ptr->size       = new_size;
            Block *next     = prev->next;
            // make merged blocks available
            release_blocks(ptr->next, prev->next);
            // relink
            ptr->next = next;
        }
        ptr = ptr->next;
    }
}
#endif

static bool init(const void *base, const void *limit, const size_t heap_blocks,
                 const size_t split_thresh, const size_t alignment)
{
    heap              = (Heap *)base;
    heap_limit        = limit;
    heap_split_thresh = split_thresh;
    heap_alignment    = alignment;
    heap_max_blocks   = heap_blocks;

    heap->free        = NULL;
    heap->used        = NULL;
    heap->fresh       = (Block *)(heap + 1);
    heap->top         = (size_t)(heap->fresh + heap_blocks);

    Block *block      = heap->fresh;
    size_t i          = heap_max_blocks - 1;
    while (i--) {
        block->next = block + 1;
        block++;
    }
    block->next = NULL;
    return true;
}

static bool free(const void *free)
{
    Block *block = heap->used;
    Block *prev  = NULL;
    while (block != NULL) {
        if (free == block->addr) {
            if (prev) {
                prev->next = block->next;
            }
            else {
                heap->used = block->next;
            }
            insert_block(block);
            compact();
            return true;
        }
        prev  = block;
        block = block->next;
    }
    return false;
}

static Block *alloc_block(size_t num)
{
    Block *ptr  = heap->free;
    Block *prev = NULL;
    size_t top  = heap->top;
    num         = (num + heap_alignment - 1) & -heap_alignment;
    while (ptr != NULL) {
        const int is_top = ((size_t)ptr->addr + ptr->size >= top) &&
                           ((size_t)ptr->addr + num <= (size_t)heap_limit);
        if (is_top || ptr->size >= num) {
            if (prev != NULL) {
                prev->next = ptr->next;
            }
            else {
                heap->free = ptr->next;
            }
            ptr->next  = heap->used;
            heap->used = ptr;
            if (is_top) {
                ptr->size = num;
                heap->top = (size_t)ptr->addr + num;
            }
            else if (heap->fresh != NULL) {
                size_t excess = ptr->size - num;
                if (excess >= heap_split_thresh) {
                    ptr->size    = num;
                    Block *split = heap->fresh;
                    heap->fresh  = split->next;
                    split->addr  = (void *)((size_t)ptr->addr + num);
                    split->size  = excess;
                    insert_block(split);
                    compact();
                }
            }
            return ptr;
        }
        prev = ptr;
        ptr  = ptr->next;
    }
    // no matching free blocks
    // see if any other blocks available
    size_t new_top = top + num;
    if (heap->fresh != NULL && new_top <= (size_t)heap_limit) {
        ptr         = heap->fresh;
        heap->fresh = ptr->next;
        ptr->addr   = (void *)top;
        ptr->next   = heap->used;
        ptr->size   = num;
        heap->used  = ptr;
        heap->top   = new_top;
        return ptr;
    }
    return NULL;
}

static void *alloc(size_t num)
{
    Block *block = alloc_block(num);
    if (block != NULL) {
        return block->addr;
    }
    return NULL;
}

static void *calloc(size_t num, size_t size)
{
    num *= size;
    Block *block = alloc_block(num);
    if (block != NULL) {
        memset(block->addr, 0, num);
        return block->addr;
    }
    return NULL;
}

static size_t count_blocks(Block *ptr)
{
    size_t num = 0;
    while (ptr != NULL) {
        num++;
        ptr = ptr->next;
    }
    return num;
}
static size_t heap_num_free()
{
    return count_blocks(heap->free);
}
static size_t heap_num_used()
{
    return count_blocks(heap->used);
}
static size_t heap_num_fresh()
{
    return count_blocks(heap->fresh);
}
static bool heap_check()
{
    return heap_max_blocks == heap_num_free() + heap_num_used() + heap_num_fresh();
}

void heap_print_stats(void)
{
    debugf(
        "HEAP\n~~~~\nFree blocks:    %lu\nUsed blocks:    %lu\nFresh blocks:   %lu\nHeap "
        "check:     %s\n",
        heap_num_free(), heap_num_used(), heap_num_fresh(), heap_check() ? "good" : "bad");
}
void *kmalloc(size_t sz)
{
#ifdef DEBUG_KMALLOC
    debugf("[kmalloc]: %lu/%lu/%lu %d\n", heap_num_free(), heap_num_used(), heap_num_fresh(),
           heap_check());
#endif
    return alloc(sz);
}
void *kcalloc(size_t n, size_t sz)
{
#ifdef DEBUG_KMALLOC
    debugf("[kcalloc]: %lu/%lu/%lu %d\n", heap_num_free(), heap_num_used(), heap_num_fresh(),
           heap_check());
#endif
    return calloc(n, sz);
}
void kfree(void *m)
{
    if (m != NULL) {
        free(m);
    }
#ifdef DEBUG_KMALLOC
    debugf("[kfree]: %lu/%lu/%lu %d\n", heap_num_free(), heap_num_used(), heap_num_fresh(),
           heap_check());
#endif
}

void heap_init(void)
{
#ifdef DEBUG_HEAP
    debugf("[heap_init]: Prior to kernel alloc: Taken: %d, Free: %d\n", page_count_taken(),
           page_count_free());
#endif
    void *start = page_znalloc(KERNEL_HEAP_PAGES);
#ifdef DEBUG_HEAP
    debugf("[heap_init]: Heap start at 0x%08lx\n", start);
    debugf("[heap_init]: After to kernel alloc: Taken: %d, Free: %d\n", page_count_taken(),
           page_count_free());
#endif

    mmu_map_range(kernel_mmu_table, KERNEL_HEAP_START_VADDR, KERNEL_HEAP_END_VADDR, (uint64_t)start,
                  MMU_LEVEL_4K, PB_READ | PB_WRITE);
    init((void *)KERNEL_HEAP_START_VADDR, (void *)KERNEL_HEAP_END_VADDR, KERNEL_HEAP_PAGES / 4, 16,
         8);

}
