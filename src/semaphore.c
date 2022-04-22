/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos.h"
#include "librertos_impl.h"

#include <string.h>

/*
 * Initialize semaphore with a custom initial value.
 *
 * It is recommended to use the functions semaphore_init_locked() and
 * semaphore_init_unlocked() instead of this.
 *
 * Parameters:
 *   - init_count: initial value of the semaphore.
 *   - max_count: maximum value of the semaphore.
 */
void semaphore_init(semaphore_t *sem, uint8_t init_count, uint8_t max_count)
{
    CRITICAL_VAL();

    LIBRERTOS_ASSERT(
        init_count <= max_count,
        init_count,
        "semaphore_init(): invalid init_count.");

    CRITICAL_ENTER();
    {
        /* Make non-zero, to be easy to spot uninitialized fields. */
        memset(sem, 0x5A, sizeof(*sem));

        sem->count = init_count;
        sem->max = max_count;
        event_init(&sem->event_unlock);
    }
    CRITICAL_EXIT();
}

/*
 * Initialize semaphore in the locked state (initial value equals zero).
 */
void semaphore_init_locked(semaphore_t *sem, uint8_t max_count)
{
    semaphore_init(sem, 0, max_count);
}

/*
 * Initialize semaphore in the unlocked state (initial value equals maximum
 * value).
 */
void semaphore_init_unlocked(semaphore_t *sem, uint8_t max_count)
{
    semaphore_init(sem, max_count, max_count);
}

/*
 * Lock semaphore.
 *
 * Returns: 1 if success, 0 otherwise.
 */
result_t semaphore_lock(semaphore_t *sem)
{
    result_t result = FAIL;
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        if (sem->count > 0)
        {
            sem->count--;
            result = SUCCESS;
        }
    }
    CRITICAL_EXIT();

    return result;
}

/*
 * Unlock semaphore.
 *
 * Returns: 1 if success, 0 otherwise.
 */
result_t semaphore_unlock(semaphore_t *sem)
{
    result_t result = FAIL;
    CRITICAL_VAL();

    CRITICAL_ENTER();

    if (sem->count < sem->max)
    {
        sem->count++;
        result = SUCCESS;
    }

    scheduler_lock();
    event_resume_task(&sem->event_unlock);
    CRITICAL_EXIT();
    scheduler_unlock();

    return result;
}

/*
 * Get the current value of the semaphore.
 */
uint8_t semaphore_get_count(semaphore_t *sem)
{
    uint8_t count;
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        count = sem->count;
    }
    CRITICAL_EXIT();

    return count;
}

/*
 * Get the maximum value of the semaphore.
 */
uint8_t semaphore_get_max(semaphore_t *sem)
{
    /* Semaphore maximum value should not change.
     * Critical section not necessary. */
    return sem->max;
}

/*
 * Suspend task on semaphore.
 */
void semaphore_suspend(semaphore_t *sem, tick_t ticks_to_delay)
{
    CRITICAL_VAL();

    CRITICAL_ENTER();

    if (sem->count == 0)
    {
        scheduler_lock();
        event_delay_task(&sem->event_unlock, ticks_to_delay);
        CRITICAL_EXIT();
        scheduler_unlock();
    }
    else
    {
        CRITICAL_EXIT();
    }
}

/*
 * Lock semaphore if unlocked, else suspend the task on the semaphore.
 *
 * Returns: 1 if successfully locked the semaphore, 0 otherwise (suspended).
 */
result_t semaphore_lock_suspend(semaphore_t *sem, tick_t ticks_to_delay)
{
    result_t result;

    result = semaphore_lock(sem);
    if (result == FAIL)
        semaphore_suspend(sem, ticks_to_delay);

    return result;
}
