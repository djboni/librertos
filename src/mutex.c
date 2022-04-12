/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos.h"
#include "librertos_impl.h"

#include <stddef.h>

enum
{
    MUTEX_UNLOCKED = 0
};

/*
 * Initialize mutex.
 */
void mutex_init(mutex_t *mtx)
{
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        mtx->locked = 0;
        mtx->task_owner = NO_TASK_PTR;
        event_init(&mtx->event_unlock);
    }
    CRITICAL_EXIT();
}

/*
 * Lock mutex.
 *
 * Returns: 1 if success, 0 otherwise.
 */
result_t mutex_lock(mutex_t *mtx)
{
    result_t result = FAIL;
    task_t *current_task;
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        current_task = librertos.current_task;

        if (mtx->locked == MUTEX_UNLOCKED || current_task == mtx->task_owner)
        {
            ++(mtx->locked);
            mtx->task_owner = current_task;
            result = SUCCESS;
        }
    }
    CRITICAL_EXIT();

    return result;
}

/*
 * Unlock mutex.
 *
 * Returns: 1 if success, 0 otherwise.
 */
result_t mutex_unlock(mutex_t *mtx)
{
    result_t result = FAIL;
    CRITICAL_VAL();

    CRITICAL_ENTER();

    if (mtx->locked != MUTEX_UNLOCKED)
    {
        --(mtx->locked);
        result = SUCCESS;
    }

    scheduler_lock();
    event_resume_task(&mtx->event_unlock);
    CRITICAL_EXIT();
    scheduler_unlock();

    return result;
}

/*
 * Check if a mutex is locked.
 */
uint8_t mutex_is_locked(mutex_t *mtx)
{
    uint8_t locked;
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        locked = mtx->locked;
    }
    CRITICAL_EXIT();

    return locked;
}

/*
 * Suspend task on mutex.
 */
void mutex_suspend(mutex_t *mtx)
{
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        if (mtx->locked != MUTEX_UNLOCKED)
            event_suspend_task(&mtx->event_unlock);
    }
    CRITICAL_EXIT();
}
