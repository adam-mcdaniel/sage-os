/**
 * @file alloc.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Global allocator.
 * @version 0.1
 * @date 2022-05-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once
#include <stdint.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

void *g_kmalloc(uint64_t size);
void *g_kcalloc(uint64_t num, uint64_t size);
void *g_kzalloc(uint64_t size);
void g_kfree(void *ptr);

extern void *(*__global_kmalloc)(uint64_t size);
extern void *(*__global_kcalloc)(uint64_t num, uint64_t size);
extern void (*__global_kfree)(void *ptr);
