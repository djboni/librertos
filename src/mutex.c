/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos.h"
#include "librertos_impl.h"

#include <string.h>

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
        /* Make non-zero, to be easy to spot uninitialized fields. */
        memset(mtx, 0x5A, sizeof(*mtx));

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

/* Unsafe. */
void task_set_priority(task_t *task, int8_t priority)
{
    struct list_t *ready_list = &librertos.tasks_ready[task->priority];
    event_t *event_list = (event_t *)task->event_node.list;

    task->priority = priority;

    if (task->sched_node.list == ready_list)
    {
        /* Put in the correct ready list. */
        list_remove(&task->sched_node);
        list_insert_first(&librertos.tasks_ready[priority], &task->sched_node);
    }

    if (event_list != NULL)
    {
        /* Put the task in the correct place in the event list. Keep it
         * suspended or delayed.
         */

        task_t *current_task = librertos.current_task;
        librertos.current_task = task;

        list_remove(&task->event_node);
        event_add_task_to_event(event_list);

        librertos.current_task = current_task;
    }
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

    if (mtx->locked == MUTEX_UNLOCKED)
    {
        task_t *owner;

        scheduler_lock();

        owner = (task_t *)mtx->task_owner;

        if (owner == NO_TASK_PTR || owner == INTERRUPT_TASK_PTR)
        {
            /* Cannot change priority of no task or interrupt. */
        }
        else
        {
            if (owner->priority != owner->original_priority)
                task_set_priority(owner, owner->original_priority);
            mtx->task_owner = NO_TASK_PTR;
        }

        event_resume_task(&mtx->event_unlock);
        CRITICAL_EXIT();
        scheduler_unlock();
    }
    else
    {
        CRITICAL_EXIT();
    }
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
void mutex_suspend(mutex_t *mtx, tick_t ticks_to_delay)
{
    CRITICAL_VAL();

    CRITICAL_ENTER();

    if (mtx->locked != MUTEX_UNLOCKED)
    {
        task_t *owner = (task_t *)mtx->task_owner;
        int8_t current_priority = librertos.current_task->priority;

        scheduler_lock();

        if (owner == NO_TASK_PTR || owner == INTERRUPT_TASK_PTR)
        {
            /* Cannot change priority of no task or interrupt. */
        }
        else
        {
            if (owner->priority < current_priority)
                task_set_priority(owner, current_priority);
        }

        event_delay_task(&mtx->event_unlock, ticks_to_delay);
        CRITICAL_EXIT();
        scheduler_unlock();
    }
    else
    {
        CRITICAL_EXIT();
    }
}

/*
 * Lock mutex if unlocked, else suspend the task on the mutex.
 *
 * Returns: 1 if successfully locked the mutex, 0 otherwise (suspended).
 */
result_t mutex_lock_suspend(mutex_t *mtx, tick_t ticks_to_delay)
{
    result_t result;

    result = mutex_lock(mtx);
    if (result == FAIL)
        mutex_suspend(mtx, ticks_to_delay);

    return result;
}
