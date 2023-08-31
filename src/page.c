#include <page.h>
#include <lock.h>
#include <stddef.h>
#include <util.h>
#include <symbols.h>
#include <debug.h>

// #define DEBUG_PAGE

// Do NOT hold the lock any longer than you have to!
Mutex page_lock;

/* 
 * Write your static functions here.
*/

void page_init(void)
{
    /* Initialize the page system. */
    uint64_t page_start = sym_start(heap);
    uint64_t page_end = sym_end(heap);
    uint64_t pages = (page_end - page_start) / PAGE_SIZE;

    mutex_spinlock(&page_lock);

    debugf("Init %lu pages.\n", pages);

    mutex_unlock(&page_lock);
}

void *page_nalloc(int n)
{
    if (n <= 0) {
        return NULL;
    }

    /* Allocate n pages */

    return NULL;
}

void *page_znalloc(int n)
{
    void *mem;
    if (n <= 0 || (mem = page_nalloc(n)) == NULL) {
        return NULL;
    }
    return memset(mem, 0, n * PAGE_SIZE);
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

