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

typedef struct PageTable PageTable;

unsigned long copy_from(void *dst, 
                        const PageTable *from_table, 
                        const void *from, 
                        unsigned long size);

unsigned long copy_to(void *to, 
                      const PageTable *to_table, 
                      const void *src, 
                      unsigned long size);
