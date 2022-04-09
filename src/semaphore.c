/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos.h"

void semaphore_init(semaphore_t *sem, uint8_t init_count, uint8_t max_count)
{
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        sem->count = init_count;
        sem->max = max_count;
    }
    CRITICAL_EXIT();
}

void semaphore_init_locked(semaphore_t *sem, uint8_t max_count)
{
    semaphore_init(sem, 0, max_count);
}

void semaphore_init_unlocked(semaphore_t *sem, uint8_t max_count)
{
    semaphore_init(sem, max_count, max_count);
}

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

result_t semaphore_unlock(semaphore_t *sem)
{
    result_t result = FAIL;
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        if (sem->count < sem->max)
        {
            sem->count++;
            result = SUCCESS;
        }
    }
    CRITICAL_EXIT();

    return result;
}

uint8_t semaphore_count(semaphore_t *sem)
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

uint8_t semaphore_max(semaphore_t *sem)
{
    /* Semaphore maximum value should not change.
     * Critical section not necessary. */
    return sem->max;
}
