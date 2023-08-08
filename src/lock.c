/**
 * @file lock.c
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Mutex routines for locking/unlocking.
 * @version 0.1
 * @date 2022-05-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <config.h>
#include <lock.h>
#include <compiler.h>

bool mutex_trylock(Mutex *mutex)
{
    int old;
    asm volatile("amoswap.w.aq %0, %1, (%2)" : "=r"(old) : "r"(MUTEX_LOCKED), "r"(mutex));
    // If old == MUTEX_LOCKED, that means the mutex was already
    // locked when we tried to lock it. That means we didn't acquire
    // it.
    return old != MUTEX_LOCKED;
}

void mutex_spinlock(Mutex *mutex)
{
    while (!mutex_trylock(mutex))
        ;
}

void mutex_unlock(Mutex *mutex)
{
    asm volatile("amoswap.w.rl zero, zero, (%0)" : : "r"(mutex));
}
