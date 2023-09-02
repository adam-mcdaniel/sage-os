#include <page.h>
#include <lock.h>
#include <stddef.h>
#include <util.h>
#include <symbols.h>
#include <debug.h>

// #define DEBUG_PAGE

// Do NOT hold the lock any longer than you have to!
Mutex page_lock;

// Bookkeeping calculation
#define BK_SIZE ((_heap_end - _heap_start) / (4096 * 4))

#define ALIGN_UP(x, a) (((x) + (a) - 1) & ~((a) - 1))

#define SET_TAKEN(index) (bookkeeping[index / 4] |= (1 << ((index % 4) * 2)))
#define CLEAR_TAKEN(index) (bookkeeping[index / 4] &= ~(1 << ((index % 4) * 2)))
#define IS_TAKEN(index) (bookkeeping[index / 4] & (1 << ((index % 4) * 2)))

#define SET_LAST(index) (bookkeeping[index / 4] |= (1 << ((index % 4) * 2 + 1)))
#define CLEAR_LAST(index) (bookkeeping[index / 4] &= ~(1 << ((index % 4) * 2 + 1)))
#define IS_LAST(index) (bookkeeping[index / 4] & (1 << ((index % 4) * 2 + 1)))

static uint8_t *bookkeeping;  // Pointer to the bookkeeping area

void page_init(void)
{
    /* Initialize the page system. */
    uint64_t page_start = sym_start(heap);
    uint64_t page_end = sym_end(heap);
    uint64_t pages = (page_end - page_start) / PAGE_SIZE;

    mutex_spinlock(&page_lock);

    // Initialize the bookkeeping area
    bookkeeping = (uint8_t *)_heap_start;
    uint64_t bk_size = BK_SIZE;
    memset(bookkeeping, 0, bk_size);

    // Mark the bookkeeping pages as taken
    for (uint64_t i = 0; i < bk_size * 4; i++) {
        SET_TAKEN(i);
    }

    debugf("Init %lu pages.\n", pages);

    mutex_unlock(&page_lock);
}

void *page_nalloc(int n)
{
    if (n <= 0) {
        return NULL;
    }

    mutex_spinlock(&page_lock);

    int consecutive = 0;
    uint64_t start_index = 0;

    for (uint64_t i = BK_SIZE * 4; i < (_heap_end - _heap_start) / PAGE_SIZE; i++) {
        if (!IS_TAKEN(i)) {
            if (consecutive == 0) {
                start_index = i;
            }
            consecutive++;
            if (consecutive == n) {
                for (uint64_t j = start_index; j < start_index + n; j++) {
                    SET_TAKEN(j);
                    if (j == start_index + n - 1) {
                        SET_LAST(j);
                    }
                }
                mutex_unlock(&page_lock);
                return (void *)(_heap_start + start_index * PAGE_SIZE);
            }
        } else {
            consecutive = 0;
        }
    }

    mutex_unlock(&page_lock);
    return NULL;
}

void *page_znalloc(int n)
{
    void *mem = page_nalloc(n);
    if (mem) {
        memset(mem, 0, n * PAGE_SIZE);
    }
    return mem;
}

void page_free(void *p)
{
    if (p == NULL) {
        return;
    }
    /* Free the page */
}

int page_count_free(void)
{
    int ret = 0;

    /* Count free pages in the bookkeeping area */

    /* Don't just take total pages and subtract taken. The point
     * of these is to detect anomalies. You are making an assumption
     * if you take total pages and subtract taken pages from it.
    */

    return ret;
}

int page_count_taken(void)
{
    int ret = 0;

    /* Count taken pages in the bookkeeping area */

    /* Don't just take total pages and subtract free. The point
     * of these is to detect anomalies. You are making an assumption
     * if you take total pages and subtract free pages from it.
    */

    return ret;
}

