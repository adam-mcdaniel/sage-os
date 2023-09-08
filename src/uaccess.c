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
                        const struct page_table *from_table, 
                        const void *from, 
                        unsigned long size)
{

    unsigned long bytes_copied = 0;

    //for each page amount of bytes or less...
    for(int i = 0; i <= size/PAGE_SIZE_4K; i++){
        unsigned long from_addr = mmu_translate(from_table,from+i); //get page addr from mmu for copying
        if (from_addr == MMU_TRANSLATE_PAGE_FAULT) {
            break;
        }
        else{
            int cpy_size = size < PAGE_SIZE_4K ? size : PAGE_SIZE_4K;   //limit copy operation to size
            memcpy(dst+i,from_addr,cpy_size); //copy page contents into destination
            bytes_copied+=cpy_size;
        }
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
                      const struct page_table *to_table, 
                      const void *src, 
                      unsigned long size)
{

    unsigned long bytes_copied = 0;


    //for each page amount of bytes or less...
    for(int i = 0; i <= size/PAGE_SIZE_4K; i++){
        unsigned long to_addr = mmu_translate(to_table,to+i); //get page addr from mmu for copying
        if (to_addr == MMU_TRANSLATE_PAGE_FAULT) {
            break;
        }
        else{
            int cpy_size = size < PAGE_SIZE_4K ? size : PAGE_SIZE_4K;   //limit copy operation to size
            memcpy(to_addr,src+i,cpy_size); //copy page contents into destination
            bytes_copied+=cpy_size;
        }
    }

    return bytes_copied;
}