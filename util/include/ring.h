/**
 * @file vring.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Ring buffer.
 * @version 0.1
 * @date 2022-05-19
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef char RingValueType;

typedef enum RingPolicy {
    RP_OVERWRITE,
    RP_DISCARD,
    RP_ERROR
} RingPolicy;

struct Ring;
typedef struct Ring Ring;


struct Ring *ring_new_with_policy(uint32_t cap, RingPolicy policy);
struct Ring *ring_new(uint32_t cap);

/**
 * @brief Get the top element from the ring without popping it off.
 *
 * @param rb the ring buffer to peek from.
 *
 * @return The value of the top element. If no element exists, this returns 0.
 */
RingValueType ring_peek(const Ring *rb);

/**
 * @brief Get the number of elements that were pushed onto the ring buffer.
 *
 * @param rb a pointer to the ring buffer.
 *
 * @return The number of elements in the ring buffer.
 */
uint32_t ring_size(const struct Ring *rb);

/**
 * @brief Gets the capacity of the ring--the number of elements that can be stored at once.
 *
 * @param rb the ring buffer to get the capacity from.
 *
 * @return the number of elements that can be stored at once.
 */
uint32_t ring_capacity(const struct Ring *rb);

/**
 * @brief Pushes data onto the ring buffer.
 *
 * @param rb the ring buffer to push onto.
 * @param data the data to push.
 *
 * @return true if the ring was pushed, false otherwise. False can only occur on the RB_ERROR
 * policy.
 */

bool ring_push(struct Ring *rb, RingValueType data);
/**
 * @brief Pops the next element off of the ring buffer.
 *
 * @param rb the ring buffer to pop.
 *
 *
 * @return The value of the top element, or 0 if the ring is empty.
 */
RingValueType ring_pop(Ring *rb);

/**
 * @brief Frees the components of a ring buffer. Does NOT free the ring buffer itself.
 *
 * @param rb the ring buffer to clean.
 */
void ring_free(struct Ring *rb);
