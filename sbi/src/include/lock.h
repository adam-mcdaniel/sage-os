/**
 * @file lock.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Mutex handling routines.
 * @version 0.1
 * @date 2022-05-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once
#include <stdbool.h>

// Mutex data structure
typedef enum {
    MUTEX_UNLOCKED,
    MUTEX_LOCKED
} Mutex;

/**
 * @brief Attempt to acquire a mutex.
 * 
 * @param mutex a pointer to the mutex.
 * @return true the mutex was acquired by us.
 * @return false we could not acquire the mutex or it is
 * already locked.
 */
bool mutex_trylock(Mutex *mutex);

/**
 * @brief Keep trying a mutex until we lock it.
 * @warning This could lead to deadlocks!
 * 
 * @param mutex a pointer to the mutex to lock.
 */
void mutex_spinlock(Mutex *mutex);

/**
 * @brief Unlock a mutex unconditionally
 * 
 * @param mutex a pointer to the mutex to unlock
 */
void mutex_unlock(Mutex *mutex);




