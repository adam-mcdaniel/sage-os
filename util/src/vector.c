#include <vector.h>
#include <alloc.h>
#include <stddef.h>
#include <util.h>

typedef struct Vector {
    uint32_t size;
    uint32_t capacity;
    uint64_t *values;
} Vector;

static Vector *vector_init(Vector *vec) {
    if (vec == NULL) {
        return NULL;
    }
    vec->size = 0;
    vec->capacity = 0;
    vec->values = NULL;
    return vec;
}

Vector *vector_new(void) {
    return vector_init((Vector *)g_kmalloc(sizeof(Vector)));
}

static Vector *vector_init_with_capacity(Vector *vec, uint32_t capacity) {
    if (vec == NULL) {
        return NULL;
    }
    vec->size = 0;
    vec->capacity = capacity;
    vec->values = (uint64_t *)g_kcalloc(capacity, sizeof(uint64_t));
    return vec;
}

Vector *vector_new_with_capacity(uint32_t capacity) {
    return vector_init_with_capacity((Vector *)g_kmalloc(sizeof(Vector)), capacity);
}

void vector_push(Vector *vec, uint64_t value) {
    vector_insert(vec, vec->size, value);
}

void vector_insert(Vector *vec, uint32_t idx, uint64_t value) {
    uint32_t i;
    vector_resize(vec, vec->size + 1);
    for (i = vec->size - 1;i > idx;i--) {
        vector_set(vec, i, vector_get_unchecked(vec, i - 1));
    }
    vector_set(vec, idx, value);
}

int vector_find(Vector *vec, uint64_t val) {
    int i;
    for (i = 0;i < (int)vec->size;i++) {
        if (vec->values[i] == val) {
            return i;
        }
    }
    return -1;
}

bool vector_set(Vector *vec, uint32_t idx, uint64_t val) {
    if (idx >= vec->size) {
        return false;
    }
    vec->values[idx] = val;
    return true;
}

bool vector_get(Vector *vec, uint32_t idx, uint64_t *val) {
    if (idx >= vec->size) {
        return false;
    }
    if (val != NULL) {
        *val = vec->values[idx];
    }
    return true;
}

uint64_t vector_get_unchecked(Vector *vec, uint32_t idx) {
    if (idx >= vec->size) {
        return 0;
    }
    return vec->values[idx];
}


void vector_resize_with_default(Vector *vec, uint32_t new_size, uint64_t def) {
    uint32_t old_size = vec->size;
    vector_resize(vec, new_size);
    if (old_size < new_size) {
        uint32_t i;
        for (i = old_size;i < new_size;i++) {
            vector_set(vec, i, def);
        }
    }
}

void vector_resize(Vector *vec, uint32_t new_size) {
    uint64_t *vals;
    uint32_t i;
    if (new_size > vec->capacity) {
        vals = (uint64_t *)g_kcalloc(new_size, sizeof(uint64_t));
        for (i = 0;i < vec->size;i++) {
            vals[i] = vec->values[i];
        }
        g_kfree(vec->values);
        vec->values = vals;
        vec->capacity = new_size;
    }        
    vec->size = new_size;    
}

void vector_reserve(Vector *vec, uint32_t new_capacity) {
    uint64_t *vals;
    uint32_t i;
    if (new_capacity > vec->capacity) {
        vals = (uint64_t *)g_kcalloc(new_capacity, sizeof(uint64_t));
        for (i = 0;i < vec->size;i++) {
            vals[i] = vec->values[i];
        }
        g_kfree(vec->values);
        vec->values = vals;
        vec->capacity = new_capacity;
    }
    else if (new_capacity > 0 && new_capacity < vec->capacity) {
        vals = (uint64_t *)g_kcalloc(new_capacity, sizeof(uint64_t));
        if (new_capacity < vec->size) {
            vec->size = new_capacity;
        }
        for (i = 0;i < vec->size;i++) {
            vals[i] = vec->values[i];
        }
        g_kfree(vec->values);
        vec->values = vals;
        vec->capacity = new_capacity;
    }
}

bool vector_remove(Vector *vec, uint32_t idx) {
    if (idx >= vec->size) {
        return false;
    }
    vec->size -= 1;
    for (;idx < vec->size;idx++) {
        vec->values[idx] = vec->values[idx + 1];
    }
    return true;
}

bool vector_remove_value(Vector *vec, uint64_t val) {
    uint32_t i;
    for (i = 0;i < vec->size;i++) {
        if (vec->values[i] == val) {
            vector_remove(vec, i);
            return true;
        }
    }
    return false;
}

void vector_clear(Vector *vec) {
    vector_resize(vec, 0);
}

int vector_binsearch_ascending(struct Vector *vec, uint64_t key) {
    int low = 0;
    int high = vector_size(vec);

    while (low < high) {
        int mid = (low + high) / 2;
        if (vector_get_unchecked(vec, mid) > key) {
            high = mid - 1;
        }
        else if (vector_get_unchecked(vec, mid) < key) {
            low = mid + 1;
        }
        else {
            return mid;
        }
    }
    return -1;
}

int vector_binsearch_descending(struct Vector *vec, uint64_t key) {
    int low = 0;
    int high = vector_size(vec) - 1;

    while (low <= high) {
        int mid = (low + high) / 2;
        if (vector_get_unchecked(vec, mid) < key) {
            high = mid - 1;
        }
        else if (vector_get_unchecked(vec, mid) > key) {
            low = mid + 1;
        }
        else {
            return mid;
        }
    }
    return -1;
}

void vector_sort(Vector *vec, VECTOR_COMPARATOR_PARAM(comp)) {
    vector_insertion_sort(vec, comp);
}

void vector_selection_sort(Vector *vec, VECTOR_COMPARATOR_PARAM(comp)) {
    uint32_t i;
    uint32_t j;
    uint32_t min_idx;
    uint64_t tmp;

    for (i = 0;i < vector_size(vec);i++) {
        min_idx = i;
        for (j = i + 1;j < vector_size(vec);j++) {
            if (!comp(vector_get_unchecked(vec, min_idx), vector_get_unchecked(vec, j))) {
                min_idx = j;
            }
        }
        tmp = vector_get_unchecked(vec, i);
        vector_set(vec, i, vector_get_unchecked(vec, min_idx));
        vector_set(vec, min_idx, tmp);
    }
}

void vector_insertion_sort(Vector *vec, VECTOR_COMPARATOR_PARAM(comp)) {
    for (uint32_t i = 1;i < vector_size(vec);i++) {
        uint32_t j = i;
        while (j > 0 && !comp(vector_get_unchecked(vec, j - 1), vector_get_unchecked(vec, j))) {
            uint64_t left = vector_get_unchecked(vec, j - 1);
            uint64_t right = vector_get_unchecked(vec, j);

            vector_set(vec, j, left);
            vector_set(vec, j - 1, right);

            j -= 1;
        }
    }
}

uint32_t vector_size(Vector *vec) {
    return vec->size;
}

uint32_t vector_capacity(Vector *vec) {
    return vec->capacity;
}

void vector_free(Vector *vec) {
    g_kfree(vec->values);
    vec->size = 0;
    vec->capacity = 0;
    g_kfree(vec);
}

VECTOR_COMPARATOR(vector_sort_signed_long_comparator_ascending) {
    return (int64_t)left <= (int64_t)right;
}
VECTOR_COMPARATOR(vector_sort_signed_long_comparator_descending) {
    return (int64_t)left >= (int64_t)right;
}
VECTOR_COMPARATOR(vector_sort_unsigned_long_comparator_ascending) {
    return left <= right;
}
VECTOR_COMPARATOR(vector_sort_unsigned_long_comparator_descending) {
    return left >= right;
}
VECTOR_COMPARATOR(vector_sort_string_comparator_ascending) {
    return strcmp((const char *)left, (const char *)right) <= 0;
}
VECTOR_COMPARATOR(vector_sort_string_comparator_descending) {
    return strcmp((const char *)left, (const char *)right) >= 0;
}

