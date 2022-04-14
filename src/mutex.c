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

/* Unsafe. */
static int8_t mutex_can_be_locked(mutex_t *mtx, task_t *current_task)
{
    return mtx->locked == MUTEX_UNLOCKED || current_task == mtx->task_owner;
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

        if (mutex_can_be_locked(mtx, current_task))
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
 */
void mutex_unlock(mutex_t *mtx)
{
    CRITICAL_VAL();

    /* To avoid failing a test, this assertion **reads** mtx->locked without the
     * protection of a critical section. See related comment in the function
     * task_suspend().
     */
    LIBRERTOS_ASSERT(
        mtx->locked != MUTEX_UNLOCKED,
        mtx->locked,
        "mutex_unlock(): mutex already unlocked.");

    CRITICAL_ENTER();

    if (mtx->locked != MUTEX_UNLOCKED)
    {
        --(mtx->locked);
    }

    scheduler_lock();
    event_resume_task(&mtx->event_unlock);
    CRITICAL_EXIT();
    scheduler_unlock();
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
        locked = !mutex_can_be_locked(mtx, librertos.current_task);
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

/*
 * Lock mutex if unlocked, else suspend the task on the mutex.
 *
 * Returns: 1 if successfully locked the mutex, 0 otherwise (suspended).
 */
result_t mutex_lock_suspend(mutex_t *mtx)
{
    result_t result;
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        result = mutex_lock(mtx);
        if (result == FAIL)
            mutex_suspend(mtx);
    }
    CRITICAL_EXIT();

    return result;
}
