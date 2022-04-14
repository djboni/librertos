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

#define NO_TASK_PRIORITY -1

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

        /* Start with scheduler locked, to avoid scheduling tasks in interrupts
         * that happen while we initialize the hardware.
         */
        librertos.scheduler_lock = 1;

        librertos.tick = 0;
        librertos.current_task = NO_TASK_PTR;

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
        node_init(&task->event_node, task);

        list_insert_last(&librertos.tasks_ready[priority], &task->sched_node);
    }
    scheduler_lock();
    CRITICAL_EXIT();
    scheduler_unlock();
}

/*
 * Start LibreRTOS, allows the scheduler to run on interrupts and when resuming
 * tasks.
 */
void librertos_start(void)
{
    --librertos.scheduler_lock;
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

    /* To avoid failing a test, this assertion **reads**
     * librertos.scheduler_lock without the protection of a critical section.
     * See related comment in the function task_suspend().
     */
    LIBRERTOS_ASSERT(
        librertos.scheduler_lock == 0,
        librertos.scheduler_lock,
        "librertos_sched(): must call librertos_start() before the scheduler.");

    /* Disable interrupts to determine the highest priority task that is ready
     * to run.
     */
    INTERRUPTS_DISABLE();

    current_task = librertos.current_task;
    current_priority = (current_task == NO_TASK_PTR) ? NO_TASK_PRIORITY
                                                     : current_task->priority;

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

task_t *interrupt_lock(void)
{
    task_t *task;

    scheduler_lock();

    task = get_current_task();
    set_current_task(INTERRUPT_TASK_PTR);

    return task;
}

void interrupt_unlock(task_t *task)
{
    set_current_task(task);
    scheduler_unlock();
}

/*
 * Process a tick timer interrupt.
 *
 * Must be called periodically by the interrupt of a timer.
 */
void librertos_tick_interrupt(void)
{
    task_t *task = interrupt_lock();
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        librertos.tick++;
    }
    CRITICAL_EXIT();

    interrupt_unlock(task);
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
 * Get the currently running task, NO_TASK_PTR (a.k.a NULL) if no task is
 * running and INTERRUPT_TASK_PTR if an interrupt is running.
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
 * Set the currently running task.
 */
void set_current_task(task_t *task)
{
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        librertos.current_task = task;
    }
    CRITICAL_EXIT();
}

/*
 * Suspend task.
 *
 * If the task is currently running, it will keep running until it returns.
 *
 * Pass CURRENT_TASK_PTR (a.k.a NULL pointer) to suspend the current task.
 */
void task_suspend(task_t *task)
{
    CRITICAL_VAL();

    /* To avoid failing a test, this assertion **reads** librertos.current_task
     * without the protection of a critical section.
     * Assertions are supposed to catch bugs on development, they should not be
     * used as run-time checks, and should be removed on production builds.
     * For this reason it is not a problem to leave it out of the critical
     * section.
     */
    LIBRERTOS_ASSERT(
        !(task == CURRENT_TASK_PTR
          && (librertos.current_task == NO_TASK_PTR
              || librertos.current_task == INTERRUPT_TASK_PTR)),
        (intptr_t)librertos.current_task,
        "task_suspend(): no task or interrupt is running.");

    CRITICAL_ENTER();
    {
        if (task == CURRENT_TASK_PTR)
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
    struct node_t *node_sched, *node_event;
    CRITICAL_VAL();

    CRITICAL_ENTER();

    list_ready = &librertos.tasks_ready[task->priority];
    node_sched = &task->sched_node;
    node_event = &task->event_node;

    if (node_sched->list != list_ready)
    {
        list_remove(node_sched);
        list_insert_last(list_ready, node_sched);

        if (node_in_list(node_event))
            list_remove(node_event);

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
uint8_t node_in_list(struct node_t *node)
{
    return node->list != NULL;
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
    list_init(&event->suspended_tasks);
}

/* Unsafe. */
void event_suspend_task(event_t *event)
{
    task_t *task = librertos.current_task;

    LIBRERTOS_ASSERT(
        !(task == NO_TASK_PTR || task == INTERRUPT_TASK_PTR),
        (intptr_t)task,
        "event_suspend_task(): no task or interrupt is running.");

    LIBRERTOS_ASSERT(
        !node_in_list(&task->event_node),
        (intptr_t)task,
        "event_suspend_task(): this task is already suspended.");

    LIBRERTOS_ASSERT(
        event->suspended_tasks.length == 0,
        (intptr_t)task,
        "event_suspend_task(): another task is suspended.");

    list_insert_last(&event->suspended_tasks, &task->event_node);
    task_suspend(task);
}

/* Unsafe. */
void event_resume_task(event_t *event)
{
    if (event->suspended_tasks.length != 0)
    {
        task_t *task = (task_t *)list_get_first(&event->suspended_tasks)->owner;
        task_resume(task);
    }
}
