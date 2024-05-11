#include <alloc.h>
#include <list.h>
#include <stddef.h>
#include <util.h>

typedef struct ListElem {
    uint64_t value;
    struct ListElem *next;
    struct ListElem *prev;
} ListElem;

typedef struct List {
    ListElem head;
} List;

static List *list_init(List *lst)
{
    lst->head.next = &lst->head;
    lst->head.prev = &lst->head;

    return lst;
}

List *list_new(void)
{
    List *m = (List *)g_kmalloc(sizeof(List));
    if (m == NULL) {
        return NULL;
    }
    return list_init(m);
}

void list_add(List *lst, uint64_t value)
{
    ListElem *l;
    l             = (ListElem *)g_kzalloc(sizeof(ListElem));
    l->next       = lst->head.next;
    l->prev       = &lst->head;
    l->next->prev = l;
    l->prev->next = l;
    l->value      = value;
}

void list_clear(List *lst)
{
    ListElem *e, *n;
    for (e = lst->head.next; e != &lst->head; e = n) {
        n = e->next;
        list_remove_elem(e);
    }
}

void list_sort(List *lst, LIST_COMPARATOR_PARAM(comp))
{
    ListElem *e;
    bool swapped;
    uint64_t tmp;

    do {
        swapped = false;
        for (e = lst->head.prev; e->prev != &lst->head; e = e->prev) {
            if (!comp(e->value, e->prev->value)) {
                swapped        = true;
                tmp            = e->value;
                e->value       = e->prev->value;
                e->prev->value = tmp;
            }
        }
    } while (swapped);
}

bool list_remove(List *lst, uint64_t value)
{
    ListElem *e;
    list_for_each(lst, e)
    {
        if (e->value == value) {
            list_remove_elem(e);
            return true;
        }
    }
    return false;
}

void list_remove_elem(ListElem *e)
{
    e->next->prev = e->prev;
    e->prev->next = e->next;
    g_kfree(e);
}

ListElem *list_find_elem(List *l, uint64_t value)
{
    ListElem *e;
    list_for_each(l, e)
    {
        if (e->value == value) {
            return e;
        }
    }
    return NULL;
}

bool list_contains(const List *lst, uint64_t value)
{
    const ListElem *e = list_find_celem(lst, value);

    return e == NULL ? false : true;
}

ListElem *list_pop_back(List *l)
{
    if (&l->head == l->head.next) {
        return NULL;
    }
    ListElem *e = l->head.next;
    list_remove_elem(e);
    return e;
}

ListElem *list_pop(List *l)
{
    if (&l->head == l->head.prev) {
        return NULL;
    }
    ListElem *e = l->head.prev;
    list_remove_elem(e);
    return e;
}

uint64_t list_size(const List *lst)
{
    uint64_t s = 0;
    const ListElem *e;
    list_for_ceach(lst, e)
    {
        s += 1;
    }
    return s;
}

void list_free(List *lst)
{
    ListElem *e, *n;
    if (lst == NULL) {
        return;
    }
    for (e = lst->head.next; e != &lst->head; e = n) {
        n = e->next;
        g_kfree(e);
    }
    g_kfree(lst);
}

bool list_elem_valid(const List *l, const ListElem *e)
{
    return e != NULL && e != &l->head;
}

const ListElem *list_find_celem(const List *l, uint64_t value) 
{
    const ListElem *e;
    list_for_ceach(l, e) 
    {
        if (e->value == value) {
            return e;
        }
    }
    return NULL;
}

ListElem *list_elem_start_ascending(List *l)
{
    return l->head.prev;
}

ListElem *list_elem_start_descending(List *l)
{
    return l->head.next;
}

ListElem *list_elem_next(ListElem *e)
{
    return e->next;
}

ListElem *list_elem_prev(ListElem *e)
{
    return e->prev;
}

uint64_t list_elem_value(const ListElem *e)
{
    return e->value;
}

const ListElem *list_celem_start_ascending(const List *l) {
    return l->head.prev;
}
const ListElem *list_celem_start_descending(const List *l) {
    return l->head.next;
}
const ListElem *list_celem_next(const ListElem *e) {
    return e->next;
}
const ListElem *list_celem_prev(const ListElem *e) {
    return e->prev;
}

// Default list comparators
LIST_COMPARATOR(list_sort_signed_long_comparator_ascending)
{
    return (int64_t)left <= (int64_t)right;
}
LIST_COMPARATOR(list_sort_signed_long_comparator_descending)
{
    return (int64_t)left >= (int64_t)right;
}
LIST_COMPARATOR(list_sort_unsigned_long_comparator_ascending)
{
    return left <= right;
}
LIST_COMPARATOR(list_sort_unsigned_long_comparator_descending)
{
    return left >= right;
}
LIST_COMPARATOR(list_sort_string_comparator_ascending)
{
    return strcmp((const char *)left, (const char *)right) <= 0;
}
LIST_COMPARATOR(list_sort_string_comparator_descending)
{
    return strcmp((const char *)left, (const char *)right) >= 0;
}
