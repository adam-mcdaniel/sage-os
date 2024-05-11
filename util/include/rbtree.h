#pragma once

#include <stdint.h>
#include <stdbool.h>

struct RBTree;
typedef struct RBTree RBTree;

struct RBTree *rb_new(void);
void rb_clear(RBTree *rb);
void rb_free(RBTree *rb);
void rb_insert(RBTree *rb, int key, uint64_t value);
void rb_delete(RBTree *rb, int key);
bool rb_find(const RBTree *rb, int key, uint64_t *value);
bool rb_min(const RBTree *rb, int *key);
bool rb_max(const RBTree *rb, int *key);
bool rb_min_val(const RBTree *rb, uint64_t *value);
bool rb_max_val(const RBTree *rb, uint64_t *value);

#define rb_insert_ptr(rb, key, value)  rb_insert(rb, key, (uint64_t)(value))
#define rb_find_ptr(rb, key, value)    rb_find(rb, key, (uint64_t*)(value))
#define rb_min_val_ptr(rb, value)      rb_min_val(rb, (uint64_t*)(value))
#define rb_max_val_ptr(rb, value)      rb_max_val(rb, (uint64_t*)(value))
#define rb_contains(rb, key)           rb_find(rb, key, NULL)
