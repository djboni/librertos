/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos_impl.h"

#include <stddef.h>
#include <string.h>

/*
 * "Unsafe" functions:
 * Some implementation functions read and change data without the protection of
 * a critical section, therefore they do not disable interrupts by themselves.
 * These functions are commented as "Unsafe" and should be used caution.
 */

/*
 * LibreRTOS state.
 */
librertos_t librertos;

/*
 * Initialize LibreRTOS state.
 *
 * Must be called **before** starting the tick timer interrupt.
 */
void librertos_init(void)
{
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        int8_t i;

        /* Make non-zero, to be easy to spot uninitialized fields. */
        memset(&librertos, 0x5A, sizeof(librertos));

        librertos.scheduler_lock = 0;
        librertos.tick = 0;
        librertos.current_task = NULL;

        for (i = 0; i < NUM_PRIORITIES; i++)
            list_init(&librertos.tasks_ready[i]);

        list_init(&librertos.tasks_suspended);
    }
    CRITICAL_EXIT();
}

/*
 * Create task.
 *
 * Parameters:
 *   - priority: integer in the range from LOW_PRIORITY (0) up to
 *     HIGH_PRIORITY (NUM_PRIORITIES - 1).
 *   - task: task information.
 *   - func: task function with prototype void task_function(void *param).
 *   - param: task parameter.
 */
void librertos_create_task(
    int8_t priority, task_t *task, task_function_t func, task_parameter_t param)
{
    CRITICAL_VAL();

    LIBRERTOS_ASSERT(
        priority >= LOW_PRIORITY && priority <= HIGH_PRIORITY,
        priority,
        "librertos_create_task(): invalid priority.");

    CRITICAL_ENTER();
    {
        /* Make non-zero, to be easy to spot uninitialized fields. */
        memset(task, 0x5A, sizeof(*task));

        task->func = func;
        task->param = param;
        task->priority = (priority_t)priority;
        node_init(&task->sched_node, task);

        list_insert_last(&librertos.tasks_ready[priority], &task->sched_node);
    }
    CRITICAL_EXIT();
}

/*
 * Run scheduled tasks.
 */
void librertos_sched(void)
{
    task_t *current_task;
    int8_t current_priority;
    int8_t some_task_ran;

    INTERRUPTS_VAL();

    /* Disable interrupts to determine the highest priority task that is ready
     * to run.
     */
    INTERRUPTS_DISABLE();

    current_task = librertos.current_task;
    current_priority = (current_task == NULL) ? -1 : current_task->priority;

    do
    {
        int8_t i;

        some_task_ran = 0;

        for (i = HIGH_PRIORITY; i > current_priority; i--)
        {
            struct node_t *node;
            task_t *task;

            if (list_is_empty(&librertos.tasks_ready[i]))
                continue;

            some_task_ran = 1;
            node = list_get_first(&librertos.tasks_ready[i]);
            task = (task_t *)node->owner;

            list_move_first_to_last(&librertos.tasks_ready[i]);

            librertos.current_task = task;

            /* Enable interrupts while running the task. */
            INTERRUPTS_ENABLE();
            task->func(task->param);
            INTERRUPTS_DISABLE();

            librertos.current_task = current_task;

            /* Break here, after running the task, and try to find another
             * higher priority task. It is necessary because a higher priority
             * task might have become ready while this was running.
             */
            break;
        }
    } while (some_task_ran != 0);

    INTERRUPTS_ENABLE();
}

/*
 * Lock scheduler, avoids preemption when using the preemptive kernel.
 */
void scheduler_lock(void)
{
    if (KERNEL_MODE == LIBRERTOS_PREEMPTIVE)
    {
        CRITICAL_VAL();

        CRITICAL_ENTER();
        ++librertos.scheduler_lock;
        CRITICAL_EXIT();
    }
}

/*
 * Unlock scheduler, causes preemption when using the preemptive kernel and a
 * higher priority tasks is ready.
 */
void scheduler_unlock(void)
{
    if (KERNEL_MODE == LIBRERTOS_PREEMPTIVE)
    {
        CRITICAL_VAL();

        CRITICAL_ENTER();
        if (--librertos.scheduler_lock == 0)
        {
            CRITICAL_EXIT();
            librertos_sched();
        }
        else
        {
            CRITICAL_EXIT();
        }
    }
}

/*
 * Process a tick timer interrupt.
 *
 * Must be called periodically by the interrupt of a timer.
 */
void librertos_tick_interrupt(void)
{
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        librertos.tick++;
    }
    CRITICAL_EXIT();
}

/*
 * Get the tick count, the number of ticks since initialization.
 *
 * Note that the tick count can overflow.
 */
tick_t get_tick(void)
{
    tick_t tick;

    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        tick = librertos.tick;
    }
    CRITICAL_EXIT();

    return tick;
}

/*
 * Get the currently running task, NULL if no task is running.
 */
task_t *get_current_task(void)
{
    task_t *task;

    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        task = librertos.current_task;
    }
    CRITICAL_EXIT();

    return task;
}

/*
 * Suspend task.
 *
 * If the task is currently running, it will run until it returns.
 *
 * Pass a NULL pointer to suspend the current task.
 */
void task_suspend(task_t *task)
{
    CRITICAL_VAL();

    /* To avoid failing a test, this assertion **reads** librertos.current_task
     * without the protection of a critical section (it does not write).
     * Assertions are supposed to catch bugs on development, they should not be
     * used as run-time checks, and should be removed on production builds.
     * For this reason it is not a problem to leave it out of the critical
     * section.
     */
    LIBRERTOS_ASSERT(
        task != NULL || librertos.current_task != NULL,
        (intptr_t)task,
        "task_suspend(): no task is running.");

    CRITICAL_ENTER();
    {
        if (task == NULL)
            task = librertos.current_task;

        list_remove(&task->sched_node);
        list_insert_first(&librertos.tasks_suspended, &task->sched_node);
    }
    CRITICAL_EXIT();
}

/*
 * Resume task.
 */
void task_resume(task_t *task)
{
    struct list_t *list_ready;
    CRITICAL_VAL();

    CRITICAL_ENTER();

    list_ready = &librertos.tasks_ready[task->priority];

    if (task->sched_node.list != list_ready)
    {
        list_remove(&task->sched_node);
        list_insert_last(list_ready, &task->sched_node);

        /* The scheduler is locked and unlocked only if a task was actually
         * resumed. When the scheduler unlocks the current task can be preempted
         * by a higher priority task.
         */
        scheduler_lock();
        CRITICAL_EXIT();
        scheduler_unlock();
    }
    else
    {
        CRITICAL_EXIT();
    }
}

/* Unsafe. */
void list_init(struct list_t *list)
{
    list->head = (struct node_t *)list;
    list->tail = (struct node_t *)list;
    list->length = 0;
}

/* Unsafe. */
void node_init(struct node_t *node, void *owner)
{
    node->next = NULL;
    node->prev = NULL;
    node->list = NULL;
    node->owner = owner;
}

/* Unsafe. */
void list_insert_after(
    struct list_t *list, struct node_t *pos, struct node_t *node)
{
    /*
     * A  x-- C --x  B
     *   <--------->
     *
     * A = pos
     * C = node
     */

    node->next = pos->next;
    node->prev = pos;

    /*
     * A <--- C ---> B
     *   <--------->
     */

    pos->next->prev = node;

    /*
     * A <--- C <--> B
     *   ---------->
     */

    pos->next = node;

    /*
     * A <--> C <--> B
     */

    node->list = list;
    list->length++;
}

/* Unsafe. */
void list_insert_before(
    struct list_t *list, struct node_t *pos, struct node_t *node)
{
    list_insert_after(list, pos->prev, node);
}

/* Unsafe. */
void list_insert_first(struct list_t *list, struct node_t *node)
{
    list_insert_after(list, LIST_HEAD(list), node);
}

/* Unsafe. */
void list_insert_last(struct list_t *list, struct node_t *node)
{
    list_insert_after(list, list->tail, node);
}

/* Unsafe. */
void list_remove(struct node_t *node)
{
    struct list_t *list = node->list;

    /*
     * A <--> C <--> B
     *
     * A = pos
     * C = node
     */

    node->next->prev = node->prev;

    /*
     * A <--> C ---> B
     *   <----------
     */

    node->prev->next = node->next;

    /*
     * A <--- C ---> B
     *   <--------->
     */

    node->next = NULL;
    node->prev = NULL;

    /*
     * A  x-- C --x  B
     *   <--------->
     */

    node->list = NULL;
    list->length--;
}

/* Unsafe. */
struct node_t *list_get_first(struct list_t *list)
{
    return list->head;
}

/* Unsafe. */
struct node_t *list_get_last(struct list_t *list)
{
    return list->tail;
}

/* Unsafe. */
uint8_t list_is_empty(struct list_t *list)
{
    return list->length == 0;
}

/* Unsafe. */
void list_move_first_to_last(struct list_t *list)
{
    struct node_t *head = list_get_first(list);
    struct node_t *tail = list_get_last(list);

    if (list->length < 2)
    {
        /* Nothing to do. */
        return;
    }

    /*
     * L <--> H <--> A <--> T <--> L
     */

    list->head = head->next;
    head->next->prev = LIST_HEAD(list);

    /*
     * L <--- H ---> A <--> T <--> L
     * L <---------> A
     */

    head->next = LIST_HEAD(list);
    head->prev = tail;

    /*
     * L <--> A <--> T <---------> L
     *               T <--- H ---> L
     */

    tail->next = head;
    list->tail = head;

    /*
     * L <--> A <--> T <--> H <--> L
     */
}

/* Unsafe. */
void event_init(event_t *event)
{
    event->task_to_resume = NULL;
}

/* Unsafe. */
void event_suspend_task(event_t *event)
{
    task_t *task = librertos.current_task;

    LIBRERTOS_ASSERT(
        task != NULL,
        (intptr_t)task,
        "event_suspend_task(): no task is running.");

    LIBRERTOS_ASSERT(
        event->task_to_resume == NULL || event->task_to_resume == task,
        (intptr_t)task,
        "event_suspend_task(): another task is suspended.");

    task_suspend(task);
    event->task_to_resume = task;
}

/* Unsafe. */
void event_resume_task(event_t *event)
{
    if (event->task_to_resume != NULL)
    {
        task_resume(event->task_to_resume);
        event->task_to_resume = NULL;
    }
}
