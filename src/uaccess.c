#include <uaccess.h>
#include <util.h>
#include <mmu.h>

/*
dst: copy destination
from: copy source
from_table: page table to translate with
size: size in bytes

copies <size> bytes from the src address into the <dst> address
*/
unsigned long copy_from(void *dst, 
                        const PageTable *from_table, 
                        const void *from, 
                        unsigned long size)
{

    unsigned long bytes_copied = 0;

    unsigned long src_start_addr = (unsigned long)from;
    unsigned long src_end_addr = (unsigned long)from+size;
    unsigned long src_first_page = ALIGN_DOWN_POT(src_start_addr, PAGE_SIZE_4K);

    // Copy the data from the source to the destination.
    // Translate the page aligned source address (for each page in the virtual addresses) using mmu_translate to get the address to copy
    // from the physical memory. Then, use memcpy to copy the data from the physical memory to the destination.
    for (unsigned long i = src_first_page; i < src_end_addr; i += PAGE_SIZE_4K) {
        void *physical_address = (void*)mmu_translate(from_table, i);

        // Is this the first page, if so, we need to copy from the offset.
        unsigned long offset = 0;

        bool is_first_page = i == src_start_addr;

        // If this is the first page:
        if (is_first_page) {
            offset = src_start_addr % PAGE_SIZE_4K;
        }

        // Is this the last page, if so, we need to copy only the remaining bytes.
        unsigned long bytes_to_copy_from_page = PAGE_SIZE_4K;
        // If the remaining bytes to copy is less than the page size, then we need to copy only the remaining bytes.
        if (i + PAGE_SIZE_4K > src_end_addr) {
            // The remaining bytes to copy is the difference between the end address and the current address.
            bytes_to_copy_from_page = src_end_addr - i;
        }

        // Copy the data from the source to the destination.
        memcpy(dst + bytes_copied, physical_address + offset, bytes_to_copy_from_page);

        // Increment the number of bytes copied.
        bytes_copied += bytes_to_copy_from_page;
    }

    return bytes_copied;
}

/*
to: copy destination
src: copy source
from_table: page table to translate with
size: size in bytes

copies <size> bytes from the src address into the <to> address
*/
unsigned long copy_to(void *to, 
                      const PageTable *to_table, 
                      const void *src, 
                      unsigned long size)
{

    unsigned long bytes_copied = 0;

    unsigned long dst_start_addr = (unsigned long)to;
    unsigned long dst_end_addr = (unsigned long)to+size;
    unsigned long dst_first_page = ALIGN_DOWN_POT(dst_start_addr, PAGE_SIZE_4K);

    // Copy the data from the source to the destination.
    // Translate the page aligned source address (for each page in the virtual addresses) using mmu_translate to get the address to copy
    // from the physical memory. Then, use memcpy to copy the data from the physical memory to the destination.
    for (unsigned long i = dst_first_page; i < dst_end_addr; i += PAGE_SIZE_4K) {
        void *physical_address = (void*)mmu_translate(to_table, i);

        // Is this the first page, if so, we need to copy from the offset.
        unsigned long offset = 0;

        bool is_first_page = i == dst_start_addr;

        // If this is the first page:
        if (is_first_page) {
            offset = dst_start_addr % PAGE_SIZE_4K;
        }

        // Is this the last page, if so, we need to copy only the remaining bytes.
        unsigned long bytes_to_copy_from_page = PAGE_SIZE_4K;
        // If the remaining bytes to copy is less than the page size, then we need to copy only the remaining bytes.
        if (i + PAGE_SIZE_4K > dst_end_addr) {
            // The remaining bytes to copy is the difference between the end address and the current address.
            bytes_to_copy_from_page = dst_end_addr - i;
        }

        // Copy the data from the source to the destination.
        memcpy(physical_address + offset, src + bytes_copied, bytes_to_copy_from_page);

        // Increment the number of bytes copied.
        bytes_copied += bytes_to_copy_from_page;
    }

    return bytes_copied;
}