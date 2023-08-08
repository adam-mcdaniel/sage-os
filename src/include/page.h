/**
 * @file page.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Full page allocation routines.
 * @version 0.1
 * @date 2022-05-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once

/**
 * @brief Initialize the page allocator. Clears the bookkeeping bytes, but does not initialize the
 * pages themselves.
 *
 */
void page_init(void);

/**
 * @brief Allocate a page. Memory is left untouched.
 * 
 * @return void* A pointer to the top of the page. NULL if error.
 */
#define page_alloc() \
    page_nalloc(1)

/**
 * @brief Allocate a number of contiguous pages. Memory is left untouched.
 * 
 * @param n The number of pages to allocate
 * @return void* A pointer to the top of the page. NULL if error.
 */
void *page_nalloc(int n);

/**
 * @brief Allocate and zero a single page.
 *
 * @return void* A page-aligned, physical address of the allocated page.
 */
#define page_zalloc() \
    page_znalloc(1)

/**
 * @brief Allocate contiguous pages and zero them.
 *
 * @param n The number of contiguous pages to allocate.
 * @return void* The page-aligned, physical address of the first page, or NULL if an error occurs.
 */
void *page_znalloc(int n);

/**
 * @brief Free a page back to the page allocator.
 *
 * @param p The page-aligned, physical address of the allocated page.
 */
void page_free(void *p);

/**
 * @brief Counts the number of free (unallocated) pages.
 *
 * @return int the number of unallocated pages.
 */
int page_count_free(void);

/**
 * @brief Counts the number of taken (allocated) pages.
 *
 * @return int the number of allocated pages
 */
int page_count_taken(void);

#define PAGE_SIZE 4096
