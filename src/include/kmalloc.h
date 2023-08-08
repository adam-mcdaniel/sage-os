/**
 * @file kmalloc.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Heap management and allocator.
 * @version 0.1
 * @date 2022-05-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once
#include <stddef.h>

/// @brief Allocate <bytes> number of bytes and return
/// a memory address to the start of these bytes.
/// Your memory address may have more bytes than requested,
/// however, you should never rely on that.
void *kmalloc(size_t bytes);
void *kcalloc(size_t n, size_t bytes);

#define kzalloc(bytes) \
    kcalloc(1, bytes)

/// @brief Return memory back to the free list that was previously allocated by kmalloc.
void kfree(void *mem);

/// @brief Before any allocations are made on the heap, this function must be called. Since MMU
/// mappings will be made, this must be passed the mmu table.
void heap_init(void);

/// @brief Print out the heap statistics. This is mainly used to check for memory leaks.
void heap_print_stats(void);
