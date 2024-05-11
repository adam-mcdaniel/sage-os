#include <alloc.h>
#include <stddef.h>

void *(*__global_kmalloc)(uint64_t size);
void *(*__global_kcalloc)(uint64_t num, uint64_t size);
void (*__global_kfree)(void *ptr);

void *g_kmalloc(uint64_t size)
{
    if (__global_kmalloc == NULL) {
        return NULL;
    }
    return __global_kmalloc(size);
}
void *g_kcalloc(uint64_t num, uint64_t size)
{
    if (__global_kcalloc == NULL) {
        return NULL;
    }
    return __global_kcalloc(num, size);
}
void *g_kzalloc(uint64_t size)
{
    return g_kcalloc(1, size);
}
void g_kfree(void *ptr)
{
    if (__global_kfree != NULL) {
        __global_kfree(ptr);
    }
}

void util_connect_galloc(void *(*malloc)(uint64_t size),
                         void *(*calloc)(uint64_t elem, uint64_t size), void (*free)(void *ptr))
{
    __global_kmalloc = malloc;
    __global_kcalloc = calloc;
    __global_kfree   = free;
}
