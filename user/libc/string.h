#pragma once

#include <stdint.h>
#include <stdbool.h>

void *memset(void *dst, char data, int size);
void *memcpy(void *dst, const void *src, int size);
bool memcmp(const void *haystack, const void *needle, int size);

int atoi(const char *st);
int strcmp(const char *l, const char *r);
int strncmp(const char *l, const char *r, int n);
int strfindchr(const char *r, char t);
int strlen(const char *s);
