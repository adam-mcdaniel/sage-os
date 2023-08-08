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
extern void *_heap_start;
extern void *_heap_end;
extern void *_stack_start;
extern void *_stack_end;
extern void *_bss_start;
extern void *_bss_end;
extern void *_data_start;
extern void *_data_end;
extern void *_text_start;
extern void *_text_end;
extern void *_rodata_start;
extern void *_rodata_end;
extern void *_memory_start;
extern void *_memory_end;

#define sym_start(segment) \
    ((unsigned long)&_##segment##_start)

#define sym_end(segment) \
    ((unsigned long)&_##segment##_end)
