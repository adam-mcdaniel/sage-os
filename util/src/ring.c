#include <ring.h>
#include <alloc.h>

typedef struct Ring {
    size_t size;
    size_t at;
    size_t capacity;
    RingPolicy policy;
    RingValueType *buffer;
} Ring;

static Ring *ring_init_with_policy(Ring *ring, uint32_t cap, RingPolicy policy) {
    ring->capacity = cap;
    ring->size = 0;
    ring->at = 0;
    ring->policy = policy;
    ring->buffer = (RingValueType*)g_kcalloc(cap, sizeof(RingValueType));
    return ring;
}

Ring *ring_new_with_policy(uint32_t cap, RingPolicy policy) {
    return ring_init_with_policy((Ring *)g_kmalloc(sizeof(Ring)), cap, policy);
}

Ring *ring_new(uint32_t cap) {
    return ring_new_with_policy(cap, RP_ERROR);
}

uint32_t ring_capacity(const Ring *rb) {
    return rb->capacity;
}

uint32_t ring_size(const Ring *rb) {
    return rb->size;
}

RingValueType ring_peek(const Ring *rb) {
    return rb->size > 0 ? rb->buffer[rb->at] : 0;
}

bool ring_push(Ring *rb, RingValueType data) {
    if (rb->size >= rb->capacity) {
        switch (rb->policy) {
            case RP_ERROR:
                return false;
            case RP_DISCARD:
                return true;
            case RP_OVERWRITE:
                rb->size = rb->capacity - 1;
                rb->at   = (rb->at + 1) % rb->capacity;
        }
    }
    rb->buffer[(rb->at + rb->size) % rb->capacity] = data;
    rb->size += 1;
    return true;
}

RingValueType ring_pop(Ring *rb)
{
    RingValueType ret;
    if (rb->size == 0) {
        return 0;
    }
    ret  = rb->buffer[rb->at];
    rb->at = (rb->at + 1) % rb->capacity;
    rb->size -= 1;
    return ret;
}

void ring_free(Ring *rb)
{
    if (rb->buffer != NULL) {
        g_kfree((void *)rb->buffer);
    }
    g_kfree(rb);
}


