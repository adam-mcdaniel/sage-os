#include <alloc.h>
#include <util.h>
#include <map.h>
#include <vector.h>
#include <list.h>
#include <stdbool.h>
#include <stdlib.h>
#include <rbtree.h>

#define RAND(mn, mx)  (mn + rand() % (mx-mn+1))

int printf(const char *fmt, ...);

int main(/*int argc, char *argv[]*/) {

    util_connect_galloc(malloc, calloc, free);

    uint64_t value;
    int minkey;

    RBTree *rb = rb_new();

    rb_insert(rb, 10, 1);
    rb_insert(rb, 30, 2);
    rb_insert(rb, 20, 3);
    rb_insert(rb, 40, 4);
    rb_insert(rb, 60, 5);
    rb_insert(rb, 50, 6);
    rb_insert(rb, 80, 7);
    rb_insert(rb, 70, 8);

    rb_delete(rb, 30);

    if (!rb_find(rb, 30, &value)) {
        printf("Unable to find key 30.\n");
    }
    else {
        printf("%d -> %lu\n", 30, value);
    }

    rb_min(rb, &minkey);
    uint64_t minval;
    rb_min_val_ptr(rb, &minval);
    rb_find(rb, minkey, &value);
    if (minval != value) {
        printf("BIG ERROR\n");
    }
    printf("Minimum key = %d (%lu/%lu).\n", minkey, minval, value);

    rb_free(rb);

    Map *m = map_new();

    map_set(m, "john", 33);
    map_set(m, "david", 221);
    map_set(m, "pauper", 312);
    map_set_int(m, 3210, 22);
    map_set_int(m, -1230, 44);
    map_set_int(m, -052, 55);
    map_set_int(m, 0, 212);

    List *keys = map_get_keys(m);
    const ListElem *e;

    list_sort(keys, list_sort_signed_long_comparator_descending);
    
    list_for_ceach_ascending(keys, e) {
        const char *k = list_celem_value_ptr(e);
        printf(" %s -> %ld\n", k, map_get_unchecked(m, k));
    }

    map_free_get_keys(keys);
    map_free(m);


    Vector *v = vector_new();
    vector_push(v, 1234);
    vector_push(v, 2133);
    for (unsigned int i = 0;i < vector_size(v);i++) {
        printf(" v[%d] = %lu\n", i, vector_get_unchecked(v, i));
    }
    vector_free(v);

    return 0;
}
