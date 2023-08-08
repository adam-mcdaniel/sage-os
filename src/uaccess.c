#include <uaccess.h>
#include <util.h>

unsigned long copy_from(void *dst, 
                        const struct page_table *from_table, 
                        const void *from, 
                        unsigned long size)
{
    // Remove the following lines. This is to squelch the
    // "unused" warnings.
    (void)dst;
    (void)from_table;
    (void)from;
    (void)size;

    unsigned long bytes_copied = 0;

    return bytes_copied;
}

unsigned long copy_to(void *to, 
                      const struct page_table *to_table, 
                      const void *src, 
                      unsigned long size)
{
    // Remove the following lines. This is to squelch the
    // "unused" warnings.
    (void)to;
    (void)to_table;
    (void)src;
    (void)size;

    unsigned long bytes_copied = 0;

    return bytes_copied;
}