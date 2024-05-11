/**
 * @file list.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Doubly-linked list utility.
 * @version 0.1
 * @date 2022-05-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define LIST_COMPARATOR_PARAM(comp) bool (*comp)(uint64_t left, uint64_t right)
#define LIST_COMPARATOR(comp)       bool comp(uint64_t left, uint64_t right)

struct List;
struct ListElem;

typedef struct List List;
typedef struct ListElem ListElem;

/**
 * @brief Allocate and initialize a new list on the heap.
 *
 * @return List* the allocated list or NULL if an error occurs.
 */
List *list_new(void);
/**
 * @brief Add an element to bottom of the list.
 *
 * @param l the list must be created and initialized
 * @param value the value to add to the bottom of the list.
 */
void list_add(List *l, uint64_t value);
#define list_add_ptr(l, value) list_add((l), (uint64_t)(value))

/**
 * @brief Remove an element by value. If there are multiple values, this only
 * removes the first one in the list order.
 *
 * @param l the list to search for the element.
 * @param value the value to remove.
 * @return true if the element was removed, false otherwise.
 */
bool list_remove(List *l, uint64_t value);
#define list_remove_ptr(l, value) list_remove((l), (uint64_t)(value))

/**
 * @brief Remove an element from the list.
 *
 * @param e the element to remove from the list
 */
void list_remove_elem(ListElem *e);
/**
 * @brief Check to see if a given value exists.
 *
 * @param l the list to search
 * @param value the value to search for
 * @return true if the value exists in the list
 * @return false if the value doesn't exist in the list
 */
bool list_contains(const List *l, uint64_t value);
#define list_contains_ptr(l, value) list_contains((l), (uint64_t)(value))
/**
 * @brief Find a value in the list and return the entire
 * list element.
 *
 * @param l the list to search
 * @param value the value to find
 * @return ListElem* the entire list element
 */
ListElem *list_find_elem(List *l, uint64_t value);
#define list_find_elem_ptr(l, value) list_find_elem((l), (uint64_t)(value))

ListElem *list_pop(List *l);
ListElem *list_pop_back(List *l);
/**
 * @brief Delete and free all list elements in a given list. This leaves
 * the list in the initialized state.
 *
 * @param l the list to clear
 */
void list_clear(List *l);
/**
 * @brief Sort the list. This uses a list comparator
 * which returns a bool and receives two uint64_t -- the left and right. The comparator
 * returns true if the left and right are properly placed, false otherwise. In other words,
 * when the comparator returns false, the compared items are swapped.
 *
 * @param lst the list to sort.
 * @param comp the comparator function.
 */
void list_sort(List *lst, LIST_COMPARATOR_PARAM(comp));
/**
 * @brief Get the size (number of nodes) in a given list.
 */
uint64_t list_size(const List *l);
/**
 * @brief Free the list and all elements from memory. This leaves the list in an undefined
 * state if it is used after this call.
 *
 * @param l the list to free
 */
void list_free(List *l);

const ListElem *list_find_celem(const List *l, uint64_t value);
#define list_find_celem_ptr(l, value) list_find_celem((l), (uint64_t)(value))

bool list_elem_valid(const List *l, const ListElem *e);
ListElem *list_elem_start_ascending(List *l);
ListElem *list_elem_start_descending(List *l);
ListElem *list_elem_next(ListElem *e);
ListElem *list_elem_prev(ListElem *e);
uint64_t list_elem_value(const ListElem *e);
#define list_elem_value_ptr(e) ((void *)list_elem_value((e)))
#define list_celem_value_ptr(e) ((const void *)list_elem_value((e)))


const ListElem *list_celem_start_ascending(const List *l);
const ListElem *list_celem_start_descending(const List *l);
const ListElem *list_celem_next(const ListElem *e);
const ListElem *list_celem_prev(const ListElem *e);

#define list_for_each_ascending_from(list, elem, start) \
    for ((elem) = (start); list_elem_valid((list), (elem)); (elem) = list_elem_prev((elem)))

#define list_for_each_descending_from(list, elem, start) \
    for ((elem) = (start); list_elem_valid((list), (elem)); (elem) = list_elem_next((elem)))

#define list_for_each_ascending(list, elem) \
    for ((elem) = list_elem_start_ascending((list)); list_elem_valid((list), (elem)); (elem) = list_elem_prev((elem)))

#define list_for_each_descending(list, elem) \
    for ((elem) = list_elem_start_descending((list)); list_elem_valid((list), (elem)); (elem) = list_elem_next((elem)))

#define list_for_each(list, elem) list_for_each_ascending(list, elem)

// Const versions
#define list_for_ceach_ascending_from(list, elem, start) \
    for ((elem) = (start); list_elem_valid((list), (elem)); (elem) = list_celem_prev((elem)))

#define list_for_ceach_descending_from(list, elem, start) \
    for ((elem) = (start); list_elem_valid((list), (elem)); (elem) = list_celem_next((elem)))

#define list_for_ceach_ascending(list, elem) \
    for ((elem) = list_celem_start_ascending((list)); list_elem_valid((list), (elem)); (elem) = list_celem_prev((elem)))

#define list_for_ceach_descending(list, elem) \
    for ((elem) = list_celem_start_descending((list)); list_elem_valid((list), (elem)); (elem) = list_celem_next((elem)))

#define list_for_ceach(list, elem) list_for_ceach_ascending(list, elem)


// Default sorter comparators
LIST_COMPARATOR(list_sort_signed_long_comparator_ascending);
LIST_COMPARATOR(list_sort_signed_long_comparator_descending);
LIST_COMPARATOR(list_sort_unsigned_long_comparator_ascending);
LIST_COMPARATOR(list_sort_unsigned_long_comparator_descending);
LIST_COMPARATOR(list_sort_string_comparator_ascending);
LIST_COMPARATOR(list_sort_string_comparator_descending);
