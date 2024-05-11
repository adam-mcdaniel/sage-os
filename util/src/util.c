#include <alloc.h>
#include <stdint.h>
#include <util.h>

void *memset(void *dst, char data, int size)
{
    int64_t i;
    long *ldst = (long *)dst;
    char *cdst;
    char l[]              = {data, data, data, data, data, data, data, data};

    int num_8_byte_copies = size / 8;
    int num_1_byte_copies = size % 8;

    for (i = 0; i < num_8_byte_copies; i++) {
        *ldst++ = *((long *)l);
    }

    cdst = (char *)ldst;

    for (i = 0; i < num_1_byte_copies; i++) {
        *cdst++ = data;
    }

    return dst;
}

void *memcpy(void *dst, const void *src, int size)
{
    int i;
    char *cdst;
    const char *csrc;
    long *ldst            = (long *)dst;
    const long *lsrc      = (long *)src;

    int num_8_byte_copies = size / 8;
    int num_1_byte_copies = size % 8;

    for (i = 0; i < num_8_byte_copies; i++) {
        *ldst++ = *lsrc++;
    }

    cdst = (char *)ldst;
    csrc = (char *)lsrc;

    for (i = 0; i < num_1_byte_copies; i++) {
        *cdst++ = *csrc++;
    }

    return dst;
}

void *memmove(void *_dst, const void *_src, int n)
{
    const char *src = _src;
    char *dst       = _dst;

    if (n == 0)
        return _dst;

    if (src < dst && src + n > dst) {
        src += n;
        dst += n;
        for (; n > 0; n -= 1) {
            *--dst = *--src;
        }
    }
    else {
        for (; n > 0; n -= 1) {
            *dst++ = *src++;
        }
    }

    return _dst;
}

int memcmp(const void *haystack, const void *needle, int size)
{
    const char *hay  = (char *)haystack;
    const char *need = (char *)needle;
    int i;

    for (i = 0; i < (size - 1); i++) {
        if (hay[i] != need[i]) {
            return hay[i] - need[i];
        }
    }

    return hay[i] - need[i];
}

int atoi(const char *st)
{
    int r = 0;
    int p = 1;
    int i;
    int l = 0;
    int n = 0;

    if (st[0] == '-') {
        st++;
        n = 1;
    }

    while (st[l] >= '0' && st[l] <= '9')
        l++;

    for (i = l - 1; i >= 0; i--) {
        r += p * (st[i] - '0');
        p *= 10;
    }

    return (n ? -r : r);
}

int strcmp(const char *l, const char *r)
{
    const unsigned char *s1 = (const unsigned char *)l;
    const unsigned char *s2 = (const unsigned char *)r;
    unsigned int c1, c2;

    do {
        c1 = (unsigned char)*s1++;
        c2 = (unsigned char)*s2++;
        if (c1 == '\0')
            return c1 - c2;
    } while (c1 == c2);

    return c1 - c2;
}

int strncmp(const char *left, const char *right, int n)
{
    unsigned int c1 = '\0';
    unsigned int c2 = '\0';

    if (n >= 4) {
        int n4 = n >> 2;
        do {
            c1 = (unsigned char)*left++;
            c2 = (unsigned char)*right++;
            if (c1 == '\0' || c1 != c2) {
                return c1 - c2;
            }

            c1 = (unsigned char)*left++;
            c2 = (unsigned char)*right++;
            if (c1 == '\0' || c1 != c2) {
                return c1 - c2;
            }
            c1 = (unsigned char)*left++;
            c2 = (unsigned char)*right++;
            if (c1 == '\0' || c1 != c2) {
                return c1 - c2;
            }
            c1 = (unsigned char)*left++;
            c2 = (unsigned char)*right++;
            if (c1 == '\0' || c1 != c2) {
                return c1 - c2;
            }
        } while (--n4 > 0);
        n &= 3;
    }

    while (n > 0) {
        c1 = (unsigned char)*left++;
        c2 = (unsigned char)*right++;
        if (c1 == '\0' || c1 != c2)
            return c1 - c2;
        n--;
    }

    return c1 - c2;
}

int strfindchr(const char *r, char t)
{
    int i = 0;
    while (r[i] != t) {
        if (r[i] == '\0') {
            return -1;
        }
        i++;
    }
    return i;
}

int strlen(const char *s)
{
    int len = 0;
    while (s[len] && ++len)
        ;
    return len;
}

char *strcpy(char *dest, const char *s)
{
    char *o = dest;
    while (*s) {
        *dest++ = *s++;
    }
    *dest = '\0';
    return o;
}

char *strncpy(char *dest, const char *s, int n)
{
    char *o = dest;
    while (*s && n-- > 0) {
        *dest++ = *s++;
    }
    *dest = '\0';
    return o;
}


char *strdup(const char *src)
{
    int len   = strlen(src);
    char *ret = (char *)g_kmalloc(len + 1);
    if (!ret) {
        return NULL;
    }
    return strcpy(ret, src);
}

bool strstartswith(const char *src, const char *start)
{
    int src_len = strlen(src);
    int start_len = strlen(start);
    int i;
    
    if (src_len < start_len) {
        return false;
    }

    for (i = 0;i < start_len;i++) {
        if (src[i] != start[i]) {
            return false;
        }
    }

    return true;
}

bool strendswith(const char *src, const char *end)
{
    int src_len = strlen(src);
    int end_len = strlen(end);
    int cmp = src_len - end_len;
    int i;
    
    if (src_len < end_len) {
        return false;
    }

    for (i = 0;i < end_len;i++) {
        if (src[i + cmp] != end[i]) {
            return false;
        }
    }

    return true;    
}
