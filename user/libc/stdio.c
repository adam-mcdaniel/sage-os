#include "stdio.h"
#include "event.h"

char getchar(void) {
    char ret;
    __asm__ volatile("mv a7, %1\necall\nmv %0, a0" : "=r"(ret) : "r"(2) : "a0", "a7");
    return ret;
}

void putchar(char c) {
    __asm__ volatile("mv a7, %0\nmv a0, %1\necall" : : "r"(1), "r"(c) : "a0", "a7");
}
