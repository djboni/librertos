/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos_impl.h"

#include <stddef.h>
#include <string.h>

/*
 * "Unsafe" functions:
 * Some implementation functions read and change data without the protection of
 * a critical section, therefore they do not disable interrupts by themselves.
 * These functions are commented as "Unsafe" and should be used with caution,
 * only inside critical sections (interrupts must be disabled).
 *
 * "Non-deterministic" functions:
 * Some implementation functions take variable time to execute, such as O(n).
 * These functions are commented as "Non-deterministic" and should be used with
 * caution, only with scheduler locked and outside critical section (interrupts
 * must be enabled).
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

    LIBRERTOS_ASSERT(
        (tick_t)-1 > 0, (tick_t)-1, "tick_t must be an unsigned integer.");

    LIBRERTOS_ASSERT(
        (difftick_t)-1 == -1,
        (difftick_t)-1,
        "difftick_t must be a signed integer.");

    LIBRERTOS_ASSERT(
        sizeof(difftick_t) == sizeof(tick_t),
        sizeof(difftick_t),
        "difftick_t must be the same size as tick_t.");

    CRITICAL_ENTER();
    {
        int8_t i;

        /* Make non-zero, to be easy to spot uninitialized fields. */
        memset(&librertos, 0x5A, sizeof(librertos));

        /* Start with scheduler locked, to avoid scheduling tasks in interrupts
         * that happen while we initialize the hardware.
         */
        librertos.scheduler_lock = 1;
        librertos.scheduler_depth = 0;

        librertos.tick = 0;
        librertos.current_task = NO_TASK_PTR;

        for (i = 0; i < NUM_PRIORITIES; i++)
            list_init(&librertos.tasks_ready[i]);

        list_init(&librertos.tasks_running);
        list_init(&librertos.tasks_suspended);
        librertos.tasks_delayed_current = &librertos.tasks_delayed[0];
        librertos.tasks_delayed_overflow = &librertos.tasks_delayed[1];
        list_init(&librertos.tasks_delayed[0]);
        list_init(&librertos.tasks_delayed[1]);
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
        task->priority = priority;
        task->original_priority = priority;
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

    librertos.scheduler_depth++;
    current_task = librertos.current_task;

    do
    {
        int8_t i;
        struct node_t *node;
        task_t *task;

        current_priority = (current_task == NO_TASK_PTR)
                             ? NO_TASK_PRIORITY
                             : current_task->priority;

        some_task_ran = 0;

        for (i = HIGH_PRIORITY; i > current_priority; i--)
        {
            if (list_is_empty(&librertos.tasks_ready[i]))
                continue;

            some_task_ran = 1;
            node = list_get_first(&librertos.tasks_ready[i]);
            task = (task_t *)node->owner;

            list_remove(node);
            list_insert_first(&librertos.tasks_running, node);

            librertos.current_task = task;

            /* Enable interrupts while running the task. */
            INTERRUPTS_ENABLE();
            task->func(task->param);
            INTERRUPTS_DISABLE();

            librertos.current_task = current_task;

            if (node->list == &librertos.tasks_running)
            {
                list_remove(node);
                list_insert_last(&librertos.tasks_ready[task->priority], node);
            }

            /* Break here, after running the task, and try to find another
             * higher priority task. It is necessary because a higher priority
             * task might have become ready while this was running.
             */
            break;
        }
    } while (some_task_ran != 0);

    librertos.scheduler_depth--;
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

static void resume_delayed_tasks(tick_t now);

/*
 * Process a tick timer interrupt.
 *
 * Must be called periodically by the interrupt of a timer.
 */
void librertos_tick_interrupt(void)
{
    task_t *task = interrupt_lock();
    tick_t now;
    CRITICAL_VAL();

    CRITICAL_ENTER();

    now = ++librertos.tick;
    resume_delayed_tasks(now);

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

static void swap_lists_of_delayed_tasks(void)
{
    struct list_t *temp = librertos.tasks_delayed_overflow;
    librertos.tasks_delayed_overflow = librertos.tasks_delayed_current;
    librertos.tasks_delayed_current = temp;
}

static void resume_list_of_tasks(struct list_t *list, tick_t now)
{
    INTERRUPTS_VAL();

    while (list->length != 0)
    {
        struct node_t *node = list_get_first(list);
        task_t *task = (task_t *)node->owner;

        if (now < task->delay_until)
            break;

        /* Check if the task is not already resumed. */
        INTERRUPTS_ENABLE();
        task_resume(task);
        INTERRUPTS_DISABLE();
    }
}

/* Non-deterministic: O(n) tasks on the list. */
static void resume_delayed_tasks(tick_t now)
{
    if (now == 0)
    {
        /* Tick overflow. */
        swap_lists_of_delayed_tasks();
        resume_list_of_tasks(librertos.tasks_delayed_overflow, MAX_DELAY);
    }

    resume_list_of_tasks(librertos.tasks_delayed_current, now);
}

/* Non-deterministic: O(n) tasks on the list. */
static struct node_t *delay_find_tick_position(struct list_t *list, tick_t tick)
{
    struct node_t *head = LIST_HEAD(list);
    struct node_t *pos;
    INTERRUPTS_VAL();

    pos = list->head;

    while (pos != head)
    {
        task_t *task = (task_t *)pos->owner;
        tick_t pos_tick = task->delay_until;

        INTERRUPTS_ENABLE();

        /* Compare outside of critical section. */
        if (tick < pos_tick)
        {
            /* Found the position: before pos. Stop. */
            INTERRUPTS_DISABLE();
            break;
        }

        INTERRUPTS_DISABLE();

        if (pos->list != list)
        {
            /* pos was removed from the list during the comparison. Restart. */
            pos = list->head;
            continue;
        }

        /* This is not the correct position. Continue. */
        pos = pos->next;
    }

    pos = pos->prev;

    return pos;
}

/* Non-deterministic: O(n) tasks on the list. */
static void task_delay_now_until(tick_t now, tick_t tick_to_wakeup)
{
    task_t *task;
    struct node_t *node, *pos;
    struct list_t *delay_list;
    CRITICAL_VAL();

    (void)tick_to_wakeup;

    scheduler_lock();
    CRITICAL_ENTER();

    task = get_current_task();
    node = &task->sched_node;
    task->delay_until = tick_to_wakeup;
    delay_list = (now < tick_to_wakeup) ? librertos.tasks_delayed_current
                                        : librertos.tasks_delayed_overflow;

    /* Suspend the task so that it can be resumed. */
    task_suspend(task);

    do
    {
        /* Non-deterministic: O(n). Calling with interrupts disabled and
         * scheduler locked.
         */
        pos = delay_find_tick_position(delay_list, tick_to_wakeup);

        /* Check if pos was removed from the list during the comparison or
         * return. Restart. */
    } while (pos != LIST_HEAD(delay_list) && pos->list != delay_list);

    /* Check if current task was not resumed. */
    if (node->list == &librertos.tasks_suspended)
    {
        /* Update the position. */
        list_remove(node);
        list_insert_after(delay_list, pos, node);
    }

    CRITICAL_EXIT();
    scheduler_unlock();
}

/*
 * Delay task for a certain amount of ticks.
 */
void task_delay(tick_t ticks_to_delay)
{
    tick_t now = get_tick();
    tick_t tick_to_wakeup = now + ticks_to_delay;
    task_delay_now_until(now, tick_to_wakeup);
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

    scheduler_lock();
    CRITICAL_ENTER();

    list_ready = &librertos.tasks_ready[task->priority];
    node_sched = &task->sched_node;
    node_event = &task->event_node;

    list_remove(node_sched);
    list_insert_last(list_ready, node_sched);

    if (node_in_list(node_event))
        list_remove(node_event);

    /* The scheduler is locked and unlocked only if a task was actually
     * resumed. When the scheduler unlocks the current task can be preempted
     * by a higher priority task.
     */
    CRITICAL_EXIT();
    scheduler_unlock();
}

/*
 * Resume all tasks.
 */
void task_resume_all(void)
{
    CRITICAL_VAL();

    scheduler_lock();
    CRITICAL_ENTER();

    resume_list_of_tasks(&librertos.tasks_suspended, MAX_DELAY);
    resume_list_of_tasks(&librertos.tasks_delayed[0], MAX_DELAY);
    resume_list_of_tasks(&librertos.tasks_delayed[1], MAX_DELAY);

    CRITICAL_EXIT();
    scheduler_unlock();
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

/* Non-deterministic: O(n) tasks on the list. */
static struct node_t *
event_find_priority_position(struct list_t *list, int8_t priority)
{
    struct node_t *head = LIST_HEAD(list);
    struct node_t *pos;
    INTERRUPTS_VAL();

    pos = list->head;

    while (pos != head)
    {
        task_t *task = (task_t *)pos->owner;
        int8_t pos_priority = task->priority;

        INTERRUPTS_ENABLE();

        /* Compare outside of critical section. */
        if (priority > pos_priority)
        {
            /* Found the position: before pos. Stop. */
            INTERRUPTS_DISABLE();
            break;
        }

        INTERRUPTS_DISABLE();

        if (pos->list != list)
        {
            /* pos was removed from the list during the comparison. Restart. */
            pos = list->head;
            continue;
        }

        /* This is not the correct position. Continue. */
        pos = pos->next;
    }

    pos = pos->prev;

    return pos;
}

/* Non-deterministic: O(n) tasks waiting for the event.
 * Call with interrupts disabled and scheduler locked.
 */
void event_add_task_to_event(event_t *event)
{
    task_t *task = librertos.current_task;
    int8_t task_priority = task->priority;
    struct node_t *pos;

    list_insert_last(&event->suspended_tasks, &task->event_node);

    do
    {
        /* Non-deterministic: O(n). Calling with interrupts disabled and
         * scheduler locked.
         */
        pos = event_find_priority_position(
            &event->suspended_tasks, task_priority);

        /* Check if pos was removed from the list during the comparison or
         * return. Restart. */
    } while (pos != LIST_HEAD(&event->suspended_tasks)
             && pos->list != &event->suspended_tasks);

    /* Check if current task was not resumed. */
    if (task->sched_node.list == &librertos.tasks_suspended)
    {
        if (pos == &task->event_node)
        {
            /* Already in the correct position. */
        }
        else
        {
            /* Update the position. */
            list_remove(&task->event_node);
            list_insert_after(&event->suspended_tasks, pos, &task->event_node);
        }
    }
}

/* Non-deterministic: O(n) tasks waiting for the event.
 * Call with interrupts disabled and scheduler locked.
 */
void event_delay_task(event_t *event, tick_t ticks_to_delay)
{
    LIBRERTOS_ASSERT(
        !(librertos.current_task == NO_TASK_PTR
          || librertos.current_task == INTERRUPT_TASK_PTR),
        (intptr_t)librertos.current_task,
        "event_delay_task(): no task or interrupt is running.");

    LIBRERTOS_ASSERT(
        !node_in_list(&librertos.current_task->event_node),
        (intptr_t)librertos.current_task,
        "event_delay_task(): this task is already suspended.");

    /* Suspend the task and insert in the end of the list so that the event
     * can resume the task.
     */
    task_suspend(CURRENT_TASK_PTR);

    event_add_task_to_event(event);

    if (ticks_to_delay != MAX_DELAY)
    {
        tick_t now = get_tick();
        tick_t tick_to_wakeup = now + ticks_to_delay;
        task_delay_now_until(now, tick_to_wakeup);
    }
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
