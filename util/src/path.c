#include <alloc.h>
#include <list.h>
#include <path.h>
#include <util.h>

const char *path_skip_slashes(const char *path)
{
    while (*path == '/') {
        path++;
    }
    return path;
}

const char *path_next_slash(const char *path)
{
    while (*path != '/' && *path != '\0') {
        path++;
    }
    return path;
}

const char *path_file_name(const char *path)
{
    int len = strlen(path);
    int i;
    for (i = len - 1; path[i] != '/' && i >= 0; i -= 1)
        ;
    return path + i + 1;
}

List *path_split(const char *path)
{
    char name[256];
    char *ptr;
    unsigned int i;
    List *v = list_new();
    while (*path != '\0') {
        path = path_skip_slashes(path);
        for (i = 0; i < (sizeof(name) - 1) && path[i] != '/' && path[i] != '\0'; i += 1) {
            name[i] = path[i];
        }
        name[i] = '\0';
        ptr     = strdup(name);
        list_add(v, (uint64_t)ptr);
        path = path_next_slash(path);
    }
    return v;
}

void path_split_free(struct List *l)
{
    ListElem *e;
    list_for_each(l, e)
    {
        void *ptr = (void *)list_elem_value(e);
        g_kfree(ptr);
    }
    list_free(l);
    g_kfree(l);
}
