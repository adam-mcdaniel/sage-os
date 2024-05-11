// map.c
// Map implementation
// Stephen Marz
// COSC562
// 7 March 2022

#include <compiler.h>
#include <alloc.h>
#include <map.h>
#include <util.h>
#include <list.h>

#define FNV1_32_OFFSET  2166136261U
#define FNV1_32_PRIME   16777619U

#define FNV1_64_OFFSET  14695981039346656037UL
#define FNV1_64_PRIME   1099511628211UL

// FNV-1a hashing
static uint64_t fnv1a_digest_64(const char *key)
{
    uint64_t digest = FNV1_64_OFFSET;
    const unsigned char *k;

    for (k = (unsigned char *)key; *k; k += 1) {
        digest = (digest ^ *k) * FNV1_64_PRIME;
    }
    return digest;
}

typedef struct Map {
    uint32_t slots;
    uint32_t size;
    struct List **values;
} Map;

typedef struct MapElem {
    char *key;
    MapValue val;
} MapElem;


// We need to store 20 digits, a possible negative sign, and the NULL-terminator
#define STR_KEY_SIZE 64
static char *map_int_to_str(char dst[], long n)
{
    char *const ret = dst;
    int i;
    int j;
    if (n == 0) {
        dst[0] = '0';
        dst[1] = '\0';
        return ret;
    }
    if (n < 0) {
        // Advance the pointer so that when we flip,
        // we don't flip the negative sign.
        *dst++ = '-';
        n = -n;
    }
    // We can get the one's place by modding by 10, but
    // this stores the number backwards.
    for (i = 0;i < (STR_KEY_SIZE - 2) && n > 0;i += 1, n /= 10) {
        dst[i] = (char)(n % 10) + '0';
    }
    // Flip the number back into order.
    for (j = 0;j < i / 2;j+=1) {
        char c;
        c = dst[j];
        dst[j] = dst[i - j - 1];
        dst[i - j - 1] = c;
    }
    dst[i] = '\0';
    return ret;
}

static MapElem *map_get_elem(Map *map, const char *key)
{
    uint64_t idx = fnv1a_digest_64(key) % map->slots;
    ListElem *e;
    MapElem *me;

    list_for_each_ascending(map->values[idx], e)
    {
        me = list_elem_value_ptr(e);
        if (!strcmp(key, me->key)) {
            return me;
        }
    }

    return NULL;
}

static const MapElem *map_get_celem(const Map *map, const char *key)
{
    uint64_t idx = fnv1a_digest_64(key) % map->slots;
    ListElem *e;
    const MapElem *me;

    list_for_each_ascending(map->values[idx], e)
    {
        me = list_celem_value_ptr(e);
        if (!strcmp(key, me->key)) {
            return me;
        }
    }

    return NULL;
}

static void mapelem_set_val(MapElem *me, MapValue val) {
    me->val = val;
}

static void mapelem_new_val(Map *map, const char *key, MapValue val) {
    MapElem *me;
    // If we get here, the key isn't there.
    uint64_t idx = fnv1a_digest_64(key) % map->slots;
    me           = (MapElem *)g_kmalloc(sizeof(MapElem));
    me->key      = strdup(key);
    me->val      = val;
    list_add_ptr(map->values[idx], me);
    map->size += 1;
}

static struct List **map_init_values(uint32_t slots) {
    struct List **lst = (struct List **)g_kcalloc(slots, sizeof(struct List *));
    for (uint32_t i = 0; i < slots; i++) {
        lst[i] = list_new();
    }
    return lst;
}

static void map_elem_free(MapElem *me) {
    g_kfree(me->key);
    g_kfree(me);
}

static void map_lst_free(ListElem *e) {
    map_elem_free(list_elem_value_ptr(e));
    list_remove_elem(e);
}

static Map *map_init(Map *map, uint32_t slots)
{
    if (map == NULL) {
        return NULL;
    }

    map->slots  = slots == 0 ? 1 : slots;
    map->values = map_init_values(map->slots);
    return map;
}

Map *map_new(void)
{
    return map_new_with_slots(DEFAULT_MAP_SIZE);
}

Map *map_new_with_slots(uint32_t slots)
{
    if (slots == 0) {
        return NULL;
    }
    return map_init((Map *)g_kmalloc(sizeof(Map)), slots);
}


uint32_t map_slots(const Map *map)
{
    return map->slots;
}

uint32_t map_size(const Map *map)
{
    return map->size;
}

void map_set(Map *map, const char *key, MapValue val)
{
    MapElem *me = map_get_elem(map, key);
    if (me != NULL) {
        mapelem_set_val(me, val);
    }
    else {
        mapelem_new_val(map, key, val);
    }
}

void map_set_int(Map *map, long ikey, MapValue val)
{
    char key[STR_KEY_SIZE];
    map_set(map, map_int_to_str(key, ikey), val);
}

MapValue map_get_unchecked(const Map *map, const char *key)
{
    MapValue ret = 0;
    map_get(map, key, &ret);
    return ret;
}

bool map_get(const Map *map, const char *key, MapValue *value)
{
    const MapElem *me = map_get_celem(map, key);
    if (me == NULL) {
        return false;
    }
    if (value != NULL) {
        *value = me->val;
    }
    return true;
}

bool map_get_int(const Map *map, long ikey, MapValue *value)
{
    char key[STR_KEY_SIZE];
    return map_get(map, map_int_to_str(key, ikey), value);
}

bool map_contains(const Map *map, const char *key)
{
    return map_get(map, key, NULL);
}

bool map_contains_int(const struct Map *map, long ikey)
{
    return map_get_int(map, ikey, NULL);
}

bool map_remove(Map *map, const char *key)
{
    uint64_t idx = fnv1a_digest_64(key) % map->slots;
    ListElem *e;

    list_for_each_ascending(map->values[idx], e)
    {
        if (!strcmp(key, ((MapElem *)list_elem_value(e))->key)) {
            map_lst_free(e);
            map->size -= 1;
            return true;
        }
    }
    return false;
}

bool map_remove_int(Map *map, long ikey)
{
    char key[STR_KEY_SIZE];
    return map_remove(map, map_int_to_str(key, ikey));
}

void map_clear(Map *map)
{
    uint32_t i;
    ListElem *e;

    for (i = 0; i < map->slots; i++) {
        list_for_each_ascending(map->values[i], e)
        {
            map_lst_free(e);
        }
    }
    map->size = 0;
}

void map_free(Map *map)
{
    uint32_t i;
    ListElem *e;

    if (map == NULL) {
        return;
    }
    for (i = 0; i < map->slots; i++) {
        list_for_each_ascending(map->values[i], e)
        {
            map_lst_free(e);
        }
        list_free(map->values[i]);
    }
    g_kfree(map->values);
    map->size = 0;
    g_kfree(map);
}

struct List *map_get_keys(const Map *map) {
    uint32_t i;
    ListElem *e;
    struct List *ret = list_new();

    for (i = 0;i < map->slots;i++) {
        list_for_each_ascending(map->values[i], e) {
            list_add(ret, (uint64_t)strdup(((MapElem *)list_elem_value(e))->key));
        }
    }

    return ret;
}

void map_free_get_keys(List *src) {
    if (src != NULL) {
        ListElem *e;
        list_for_each_ascending(src, e) {
            g_kfree(list_elem_value_ptr(e));
        }
        list_free(src);
    }
}
