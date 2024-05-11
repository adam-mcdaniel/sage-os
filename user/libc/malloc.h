#include <stdint.h>

void salloc_init(uint8_t *start, uint8_t *end);
void *salloc(uint32_t bytes);
void sfree(void *ptr);

void init_malloc(void *start, uint32_t size);
void *malloc(uint32_t size);
void free(void *ptr);
