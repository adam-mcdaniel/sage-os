/**
 * @file map.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief struct Map data structure storage routines.
 * @version 0.1
 * @date 2022-05-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>

#define DEFAULT_MAP_SIZE 64

struct List;
struct Map;
struct MapElem;

typedef uint64_t MapValue;
typedef struct Map Map;
typedef struct MapElem MapElem;

/**
 * Construct a new map using the heap with the default number
 * of hash slots.
 *
 * @return a pointer to the new map on the heap.
 */
struct Map *map_new(void);
/**
 * Construct a new map using the heap.
 *
 * @param slots the number of distinct hash table slots.
 *
 * @return a pointer to the new map on the heap.
 */
struct Map *map_new_with_slots(uint32_t slots);
/**
 * Get the number of hash table slots for this map.
 *
 * @param map the map to retrieve the slots.
 *
 * @return the number of hash table slots.
 */
uint32_t map_slots(const struct Map *map);
/**
 * Get the number of items in the map regardless of position.
 *
 * @param map the map to retrieve the size.
 *
 * @return the number of items in the map.
 */
uint32_t map_size(const struct Map *map);
/**
 * Set a key/value pair in the map. This will use a hash value
 * to determine the index. If there is a collision, the value is
 * added into a list of the same index.
 *
 * If the key already exists, the value is overwritten.
 * If the key does not exist, the key/value pair is created.
 *
 * @param map the map to set a value.
 * @param key the key to set the value of.
 * @param val the value to set for the given key.
 */
void map_set(struct Map *map, const char *key, MapValue val);
/**
 * Set a key/value pair in the map. This will use a hash value
 * to determine the index. If there is a collision, the value is
 * added into a list of the same index.
 *
 * If the key already exists, the value is overwritten.
 * If the key does not exist, the key/value pair is created.
 *
 * @param map the map to set a value.
 * @param key the key to set the value of.
 * @param val the value to set for the given key.
 */
void map_set_int(struct Map *map, long key, MapValue val);
/**
 * @brief
 * Get the value of a given key without validating the return.
 *
 * WARNING: This will return 0 if the key is not
 * found. Use `map_contains` to first determine if the key is in the map since
 * 0 is a valid value for a key.
 *
 * @param map the map to search for the given key.
 * @param key the key of the value to find.
 *
 * @return the value of the given key or 0 if the key was not found.
 */
MapValue map_get_unchecked(const struct Map *map, const char *key);
/**
 * @brief
 * Get the value of a given key and check the value.
 *
 * @param map the map to search for the given key.
 * @param key the key of the value to find.
 * @param val a pointer to where to put the value. If this is null, this
 * function has the same effect as map_contains.
 *
 * @return true if the value was found regardless if val is NULL or not, false otherwise.
 */
bool map_get(const struct Map *map, const char *key, MapValue *val);
/**
 * @brief
 * Get the value of a given key and check the value.
 *
 * @param map the map to search for the given key.
 * @param key the key of the value to find.
 * @param val a pointer to where to put the value. If this is null, this
 * function has the same effect as map_contains.
 *
 * @return true if the value was found regardless if val is NULL or not, false otherwise.
 */
bool map_get_int(const struct Map *map, long key, MapValue *val);
/**
 * Remove a key/value pair from the map.
 *
 * @param map the map itself.
 * @param key the key of the key/value pair to remove.
 *
 * @return true if the key/value pair was removed, false otherwise.
 */
bool map_remove(struct Map *map, const char *key);
/**
 * Remove a key/value pair from the map.
 *
 * @param map the map itself.
 * @param key the key of the key/value pair to remove.
 *
 * @return true if the key/value pair was removed, false otherwise.
 */
bool map_remove_int(struct Map *map, long key);
/**
 * Clear all key/value pairs in the map.
 *
 * @param map the map to clear.
 */
void map_clear(struct Map *map);
/**
 * Test if a given key is in the map.
 *
 * @param map the map to test.
 * @param key the key to test.
 *
 * @return true if the key is in the map, false otherwise.
 */
bool map_contains(const struct Map *map, const char *key);
/**
 * Test if a given key is in the map.
 *
 * @param map the map to test.
 * @param key the key to test.
 *
 * @return true if the key is in the map, false otherwise.
 */
bool map_contains_int(const struct Map *map, long key);
/**
 * Free the map by freeing all elements, keys, etc.
 *
 * NOTE: This does NOT free the pointer passed. This is used to support
 * stack based struct Map allocations. So, if you created the map on the heap,
 * be sure to free it!
 *
 * @param map the map to free.
 */
void map_free(struct Map *map);
/**
 * Get all of the map keys and put them into a list. Since this is a map,
 * these are in hash order, meaning you cannot predict the order of the keys.
 * 
 * @param map the map to get all of the keys from.
 * 
 * @return the list of keys.
*/
struct List *map_get_keys(const struct Map *map);
/**
 * Free a list of keys given from a map_get_keys call.
 * 
 * @param src the list to free from map_get_keys.
*/
void map_free_get_keys(struct List *src);
