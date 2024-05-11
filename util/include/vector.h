/**
 * @file vector.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Implement a contiguous vector.
 * @version 0.1
 * @date 2022-05-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

struct Vector;
typedef struct Vector Vector;

#define VECTOR_CAPACITY_PUSH 5

#define VECTOR_COMPARATOR_PARAM(comp) bool (*comp)(uint64_t left, uint64_t right)
#define VECTOR_COMPARATOR(comp)       bool comp(uint64_t left, uint64_t right)

struct Vector *vector_new(void);
struct Vector *vector_new_with_capacity(uint32_t capacity);

void vector_push(struct Vector *vec, uint64_t value);
void vector_insert(struct Vector *vec, uint32_t idx, uint64_t value);
void vector_resize(struct Vector *vec, uint32_t new_size);
void vector_resize_with_default(struct Vector *vec, uint32_t new_size, uint64_t def);
void vector_reserve(struct Vector *vec, uint32_t new_capacity);
void vector_clear(struct Vector *vec);
int vector_find(struct Vector *vec, uint64_t val);
bool vector_set(struct Vector *vec, uint32_t idx, uint64_t val);
bool vector_get(struct Vector *vec, uint32_t idx, uint64_t *val);
bool vector_remove(struct Vector *vec, uint32_t idx);
bool vector_remove_value(struct Vector *vec, uint64_t val);
uint64_t vector_get_unchecked(struct Vector *vec, uint32_t idx);
void vector_sort(struct Vector *vec, VECTOR_COMPARATOR_PARAM(comp));
void vector_insertion_sort(struct Vector *vec, VECTOR_COMPARATOR_PARAM(comp));
void vector_selection_sort(struct Vector *vec, VECTOR_COMPARATOR_PARAM(comp));
int vector_binsearch_ascending(struct Vector *vec, uint64_t key);
int vector_binsearch_descending(struct Vector *vec, uint64_t key);
uint32_t vector_size(struct Vector *vec);
uint32_t vector_capacity(struct Vector *vec);
void vector_free(struct Vector *vec);

#define vector_push_ptr(vec, value)           vector_push(vec, (uint64_t)value)
#define vector_insert_ptr(vec, idx, value)    vector_insert(vec, idx, (uint64_t)value)
#define vector_find_ptr(vec, val)             vector_find(vec, (uint64_t)val)
#define vector_set_ptr(vec, idx, val)         vector_set(vec, idx, (uint64_t)val)
#define vector_get_ptr(vec, idx, val)         vector_get(vec, idx, (uint64_t*)(val))
#define vector_get_ptr_unchecked(vec, idx)    (void*)vector_get_unchecked(vec, idx)
#define vector_remove_val_ptr(vec, val)       vector_remove_value(vec, (uint64_t)val)

VECTOR_COMPARATOR(vector_sort_signed_long_comparator_ascending);
VECTOR_COMPARATOR(vector_sort_signed_long_comparator_descending);
VECTOR_COMPARATOR(vector_sort_unsigned_long_comparator_ascending);
VECTOR_COMPARATOR(vector_sort_unsigned_long_comparator_descending);
VECTOR_COMPARATOR(vector_sort_string_comparator_ascending);
VECTOR_COMPARATOR(vector_sort_string_comparator_descending);

