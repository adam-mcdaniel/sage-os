#include <page.h>
#include <lock.h>
#include <stddef.h>
#include <stdint.h>
#include <util.h>
#include <symbols.h>
#include <debug.h>
#include <mmu.h>

// #define PAGE_DEBUG
#ifdef PAGE_DEBUG
#define debugf(...) debugf(__VA_ARGS__)
#else
#define debugf(...)
#endif

// Do NOT hold the lock any longer than you have to!
Mutex page_lock;

// Bookkeeping calculation
// #define ALIGN_UP(x, a) (((x) + (a) - 1) & ~((a) - 1))

static uint8_t *bookkeeping;  // Pointer to the bookkeeping area

// For some reason, the macros didn't work for me, so I used the static functions like Marz said and it works.
static void set_taken(uint64_t index)
{
    bookkeeping[index / 4] |= (1 << ((index % 4) * 2));
}

static void clear_taken(uint64_t index)
{
    bookkeeping[index / 4] &= ~(1 << ((index % 4) * 2));
}

static int is_taken(uint64_t index)
{
    return bookkeeping[index / 4] & (1 << ((index % 4) * 2));
}

static void set_last(uint64_t index)
{
    bookkeeping[index / 4] |= (1 << ((index % 4) * 2 + 1));
}

static void clear_last(uint64_t index)
{
    bookkeeping[index / 4] &= ~(1 << ((index % 4) * 2 + 1));
}

static int is_last(uint64_t index)
{
    return bookkeeping[index / 4] & (1 << ((index % 4) * 2 + 1));
}

void page_init(void)
{
    /* Initialize the page system. */
    bookkeeping = (uint8_t*)sym_start(heap);
    
    // Print bookkeeping area
    mutex_spinlock(&page_lock);

    // Initialize the bookkeeping area
    memset(bookkeeping, 0, BK_SIZE_IN_BYTES);
    // Mark the bookkeeping pages as taken
    for (uint64_t i = 0; i < BK_SIZE_IN_PAGES; i++) {
        set_taken(i);
    }
    set_last(BK_SIZE_IN_PAGES-1);

    debugf("page_init: bookkeeping area initialized\n");
    debugf("page_init: bookkeeping area starts at 0x%08lx\n", bookkeeping);
    debugf("page_init: bookkeeping area ends at 0x%08lx\n", bookkeeping + BK_SIZE_IN_BYTES);
    mutex_unlock(&page_lock);

    // Print out the bookkeeping area's contents
    logf(LOG_INFO, "Page Init: 0x%08lx -> 0x%08lx\n", bookkeeping, bookkeeping + BK_SIZE_IN_BYTES);
    logf(LOG_INFO, "  Heap: 0x%08lx -> 0x%08lx", bookkeeping + BK_SIZE_IN_BYTES, bookkeeping + HEAP_SIZE_IN_BYTES);
    logf(LOG_INFO, "  Heap size: 0x%lx bytes, %lu pages\n", HEAP_SIZE_IN_BYTES, HEAP_SIZE_IN_PAGES);
    logf(LOG_INFO, "  Bookkeeping size: 0x%lx bytes, %lu pages\n", BK_SIZE_IN_BYTES, BK_SIZE_IN_PAGES);
    logf(LOG_INFO, "  Taken pages: %lu\n", page_count_taken());
    logf(LOG_INFO, "  Free pages: %lu\n", page_count_free());
    
    // This code tests the page allocator
    // void *a = page_nalloc(1);

    // logf(LOG_INFO, "Page Init: 0x%08lx -> 0x%08lx\n", bookkeeping, bookkeeping + BK_SIZE_IN_BYTES);
    // logf(LOG_INFO, "  Heap size: 0x%lx bytes, %lu pages\n", HEAP_SIZE_IN_BYTES, HEAP_SIZE_IN_PAGES);
    // logf(LOG_INFO, "  Bookkeeping size: 0x%lx bytes, %lu pages\n", BK_SIZE_IN_BYTES, BK_SIZE_IN_PAGES);
    // logf(LOG_INFO, "  Taken pages: %lu\n", page_count_taken());
    // logf(LOG_INFO, "  Free pages: %lu\n", page_count_free());

    // void *b = page_nalloc(2);

    // logf(LOG_INFO, "Page Init: 0x%08lx -> 0x%08lx\n", bookkeeping, bookkeeping + BK_SIZE_IN_BYTES);
    // logf(LOG_INFO, "  Heap size: 0x%lx bytes, %lu pages\n", HEAP_SIZE_IN_BYTES, HEAP_SIZE_IN_PAGES);
    // logf(LOG_INFO, "  Bookkeeping size: 0x%lx bytes, %lu pages\n", BK_SIZE_IN_BYTES, BK_SIZE_IN_PAGES);
    // logf(LOG_INFO, "  Taken pages: %lu\n", page_count_taken());
    // logf(LOG_INFO, "  Free pages: %lu\n", page_count_free());

    // void *c = page_nalloc(4);

    // logf(LOG_INFO, "Page Init: 0x%08lx -> 0x%08lx\n", bookkeeping, bookkeeping + BK_SIZE_IN_BYTES);
    // logf(LOG_INFO, "  Heap size: 0x%lx bytes, %lu pages\n", HEAP_SIZE_IN_BYTES, HEAP_SIZE_IN_PAGES);
    // logf(LOG_INFO, "  Bookkeeping size: 0x%lx bytes, %lu pages\n", BK_SIZE_IN_BYTES, BK_SIZE_IN_PAGES);
    // logf(LOG_INFO, "  Taken pages: %lu\n", page_count_taken());
    // logf(LOG_INFO, "  Free pages: %lu\n", page_count_free());

    // page_free(a);

    // logf(LOG_INFO, "Page Init: 0x%08lx -> 0x%08lx\n", bookkeeping, bookkeeping + BK_SIZE_IN_BYTES);
    // logf(LOG_INFO, "  Heap size: 0x%lx bytes, %lu pages\n", HEAP_SIZE_IN_BYTES, HEAP_SIZE_IN_PAGES);
    // logf(LOG_INFO, "  Bookkeeping size: 0x%lx bytes, %lu pages\n", BK_SIZE_IN_BYTES, BK_SIZE_IN_PAGES);
    // logf(LOG_INFO, "  Taken pages: %lu\n", page_count_taken());
    // logf(LOG_INFO, "  Free pages: %lu\n", page_count_free());

    // page_free(b);

    // logf(LOG_INFO, "Page Init: 0x%08lx -> 0x%08lx\n", bookkeeping, bookkeeping + BK_SIZE_IN_BYTES);
    // logf(LOG_INFO, "  Heap size: 0x%lx bytes, %lu pages\n", HEAP_SIZE_IN_BYTES, HEAP_SIZE_IN_PAGES);
    // logf(LOG_INFO, "  Bookkeeping size: 0x%lx bytes, %lu pages\n", BK_SIZE_IN_BYTES, BK_SIZE_IN_PAGES);
    // logf(LOG_INFO, "  Taken pages: %lu\n", page_count_taken());
    // logf(LOG_INFO, "  Free pages: %lu\n", page_count_free());

    // page_free(c);

    // logf(LOG_INFO, "Page Init: 0x%08lx -> 0x%08lx\n", bookkeeping, bookkeeping + BK_SIZE_IN_BYTES);
    // logf(LOG_INFO, "  Heap size: 0x%lx bytes, %lu pages\n", HEAP_SIZE_IN_BYTES, HEAP_SIZE_IN_PAGES);
    // logf(LOG_INFO, "  Bookkeeping size: 0x%lx bytes, %lu pages\n", BK_SIZE_IN_BYTES, BK_SIZE_IN_PAGES);
    // logf(LOG_INFO, "  Taken pages: %lu\n", page_count_taken());
    // logf(LOG_INFO, "  Free pages: %lu\n", page_count_free());
}


void *page_nalloc(uint64_t n)
{
    if (n <= 0) {
        return NULL;
    }

    mutex_spinlock(&page_lock);

    uint64_t start = 0;
    uint64_t consecutive = 0;

    for (uint64_t i = 0; i < HEAP_SIZE_IN_PAGES; i++) {
        if (!is_taken(i) && !is_last(i)) {
            if (consecutive == 0) {
                start = i;
            }

            consecutive++;

            if (consecutive >= n) {
                // debugf("page_nalloc: found %d consecutive pages starting at 0x%08lx\n", n, start);
                for (int j = 0; j < n; j++) {
                    // debugf("page_nalloc: marking page 0x%08lx as taken\n", start + j);
                    set_taken(start + j);
                    if (j == n - 1) {
                        set_last(start + j);
                    }
                }
                // debugf("page_nalloc: marking page 0x%08lx as last\n", start + n - 1);

                mutex_unlock(&page_lock);
                // debugf("Found free %d pages at #%d, %d\n", n, start, i);
                void *result = (void*)((uint64_t)bookkeeping + ((uint64_t)start * PAGE_SIZE));

                debugf("Found %d free pages at %p\n", n, result);
                // Print remaining pages
                uint64_t remaining = page_count_free();
                debugf("Remaining pages: %lu\n", remaining);

                return result;
            }
        } else {
            // debugf("page_nalloc: page 0x%08lx is taken\n", i);
            consecutive = 0;
        }
    }

    mutex_unlock(&page_lock);
    return NULL;
}

void *page_znalloc(uint64_t n)
{
    if (n <= 0) {
        return NULL;
    }
    
    void *mem = page_nalloc(n);
    if (mem) {
        // debugf("page_znalloc: zeroing out %d pages starting at 0x%08lx\n", n, mem);
        memset(mem, 0, n * PAGE_SIZE);
    }
    return mem;
}

uint64_t page_to_index(void *page) {
    return ((uint64_t)page - (uint64_t)bookkeeping) / PAGE_SIZE;
}

void *index_to_page(uint64_t idx) {
    return (void*)(idx * PAGE_SIZE + (uint64_t)bookkeeping);
}

void page_free(void *p)
{
    if (p == NULL) {
        return;
    }
    /* Free the page */
    // uint64_t x = ((uint64_t)p - (uint64_t)bookkeeping) / PAGE_SIZE;
    uint64_t x = page_to_index(p);
    // debugf("page_free: freeing page %lu at address 0x%p\n", x, p);

    mutex_spinlock(&page_lock);


    if (!is_taken(x)) {
        // logf(LOG_ERROR, "page_free: page 0x%08lx is already free!\n", x);
        mutex_unlock(&page_lock);
        return;
    }

    // Clear all the pages starting at the index until the last page
    while (is_taken(x) && !is_last(x)) {
        clear_taken(x);
        x++;
    }
    clear_taken(x);
    clear_last(x);


    mutex_unlock(&page_lock);
}

uint64_t page_count_free(void)
{
    uint64_t ret = 0;

    /* Count free pages in the bookkeeping area */

    /* Don't just take total pages and subtract taken. The point
     * of these is to detect anomalies. You are making an assumption
     * if you take total pages and subtract taken pages from it.
    */

    mutex_spinlock(&page_lock);
    for (uint64_t i = 0; i < HEAP_SIZE_IN_PAGES; i++) {
       if (!is_taken(i)) {
           ret++;
       }
    }
    mutex_unlock(&page_lock);

    return ret;
}

uint64_t page_count_taken(void)
{
    uint64_t ret = 0;

    /* Count taken pages in the bookkeeping area */

    /* Don't just take total pages and subtract free. The point
     * of these is to detect anomalies. You are making an assumption
     * if you take total pages and subtract free pages from it.
    */

    mutex_spinlock(&page_lock);
    for (uint64_t i = 0; i < HEAP_SIZE_IN_PAGES; i++) {
       if (is_taken(i)) {
           ret++;
       }
    }
    mutex_unlock(&page_lock);

    return ret;
}
