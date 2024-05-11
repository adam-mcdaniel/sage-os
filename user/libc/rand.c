#include "rand.h"


static uint64_t seed = 0;
uint64_t rand() {
    seed = (seed * 0x5DEECE66D + 0xB) & ((1ULL << 48) - 1);
    return seed >> 16;
}
void srand(uint64_t new_seed) {
    seed = new_seed;
}