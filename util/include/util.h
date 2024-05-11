/**
 * @file util.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Helper utilities for your OS.
 * @version 0.1
 * @date 2022-05-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Bit fields
#define BIT_TEST(bitset, index)   (((bitset) >> (index)) & 1)
#define BIT_SET(bitset, index)    ((bitset) | (1UL << (index)))
#define BIT_CLEAR(bitset, index)  ((bitset) & ~(1UL << (index)))

// These functions are analogous to their libstdc counterparts.
void *memset(void *dst, char data, int size);
void *memcpy(void *dst, const void *src, int size);
void *memmove(void *dst, const void *src, int size);
int memcmp(const void *haystack, const void *needle, int size);

// These functions more or less follow libstdc conventions, but that
// is not guaranteed.
int atoi(const char *st);
int strcmp(const char *l, const char *r);
int strncmp(const char *l, const char *r, int n);
int strfindchr(const char *r, char t);
int strlen(const char *s);
char *strcpy(char *dest, const char *s);
char *strncpy(char *dest, const char *s, int n);
char *strdup(const char *src);
bool strstartswith(const char *src, const char *start);
bool strendswith(const char *src, const char *end);

/**
 * @brief Align value x up to the power of two value y. This returns
 * an undefined value if y is NOT a power of two.
 */
#define ALIGN_UP_POT(x, y)   (((x) + (y)-1) & -(y))
/**
 * @brief Align value x down to the power of two value y. This returns
 * an undefined value if y is NOT a power of two.
 */
#define ALIGN_DOWN_POT(x, y) ((x) & -(y))

/**
 * @brief Connect the utility library to the heap allocators.
 * 
 * This must be done BEFORE using the utility library!
 * 
 * @param malloc the kmalloc function
 * @param calloc the kcalloc function
 * @param free the kfree function
*/
void util_connect_galloc(void *(*malloc)(uint64_t size),
                         void *(*calloc)(uint64_t elem, uint64_t size), void (*free)(void *ptr));
