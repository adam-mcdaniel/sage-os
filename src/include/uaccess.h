/**
 * @file symbols.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Utilities to read linker script exports.
 * @version 0.1
 * @date 2022-05-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once

struct page_table;
unsigned long copy_from(void *dst, 
                        const struct page_table *from_table, 
                        const void *from, 
                        unsigned long size);

unsigned long copy_to(void *to, 
                      const struct page_table *to_table, 
                      const void *src, 
                      unsigned long size);
