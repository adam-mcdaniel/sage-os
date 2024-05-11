#include <stdint.h>
#include "malloc.h"
#include "stdio.h"
#include "string.h"

#define NULL ((void*)0)


uint8_t *heap_start, *heap_end;
void salloc_init(uint8_t *start, uint8_t *end) {
    heap_start = start;
    heap_end = end;
    // printf("salloc_init: %p - %p\n", heap_start, heap_end);
}


void init_malloc(void *start, uint32_t size) {
    // printf("init_malloc: %p - %p\n", start, ((uint8_t*)start) + size);
    salloc_init(start, ((uint8_t*)start) + size);
}

void *salloc(uint32_t bytes) {
	// printf("salloc: %d\n", bytes);
	uint64_t words = (bytes + 128) / 8;
    uint64_t *ret = (uint64_t*)heap_start;
    while (*ret != 0) {
		// printf("salloc: %p taken with reserved words=%ld, bytes=%ld\n", ret, *ret, *ret * 8);
        ret += *ret;
        if ((uintptr_t)ret >= (uintptr_t)heap_end) {
            // printf("salloc: out of memory\n");
            // printf("salloc: tried to allocate %d bytes\n", bytes);
            return NULL;
        }
    }
    *ret = words;
    ret++;
	// printf("salloc: %p (remaining=%d)\n", ret, (uint32_t)(heap_end - ((uint8_t*)ret + bytes)));
    return ret;
}

void sfree(void *ptr) {
	// printf("sfree: %p\n", ptr);
	if ((uintptr_t)ptr < (uintptr_t)heap_start || (uintptr_t)ptr >= (uintptr_t)heap_end) {
		// printf("sfree: invalid pointer\n");
		return;
	}

    uint64_t *p = ptr;
    uint64_t words = *(p - 1);
    memset(p - 1, 0, words / 8);

    return;
}


void *malloc(uint32_t size) {
    return salloc(size);
    // void *ret;
    // __asm__ volatile("mv a7, %1\nmv a0, %2\necall\nmv %0,a0"
    //                  : "=r"(ret)
    //                  : "r"(SYS_MALLOC), "r"(size)
    //                  : "a0", "a7");
    // return ret;
}

void free(void *ptr) {
    sfree(ptr);
    // __asm__ volatile("mv a7, %0\nmv a0, %1\necall" : : "r"(SYS_FREE), "r"(ptr) : "a0", "a7");
}