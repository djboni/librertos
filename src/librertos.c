/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos_impl.h"

#include <stddef.h>
#include <string.h>

enum {
    MUTEX_UNLOCKED = 0,
    TASK_NOT_RUNNING = 0,
    TASK_RUNNING = 1
};

/*
 * LibreRTOS state.
 */
librertos_t librertos;

/*
 * Initialize LibreRTOS state.
 *
 * Must be called before starting the tick timer interrupt or any other
 * interrupt that uses LibreRTOS functions.
 */
void librertos_init(void) {
    int8_t i;
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

    /* Make non-zero, to be easy to spot uninitialized fields. */
    memset(&librertos, NONZERO_INITVAL, sizeof(librertos));

    /* Start with scheduler locked, to avoid scheduling tasks in interrupts
     * while initializing the hardware and when creating the tasks on
     * initialization.
     */
    librertos.scheduler_lock = 1;

    librertos.scheduler_depth = 0;
    librertos.tick = 0;
    librertos.current_task = NO_TASK_PTR;

    for (i = 0; i < NUM_PRIORITIES; i++)
        list_init(&librertos.tasks_ready[i]);

    list_init(&librertos.tasks_suspended);
    librertos.tasks_delayed_current = &librertos.tasks_delayed[0];
    librertos.tasks_delayed_overflow = &librertos.tasks_delayed[1];

    for (i = 0; i < 2; i++)
        list_init(&librertos.tasks_delayed[i]);

    CRITICAL_EXIT();
}

/*
 * Create a task.
 *
 * @param priority Task priority. Integer in the range from LOW_PRIORITY to
 * HIGH_PRIORITY (0 to NUM_PRIORITIES-1).
 * @param task Task scruct information.
 * @param func Function executed by the task, with prototype
 * void task_function(void *param).
 * @param param Parameter passed to the task function.
 */
void librertos_create_task(
    int8_t priority, task_t *task, task_function_t func, task_parameter_t param) {
    CRITICAL_VAL();

    LIBRERTOS_ASSERT(
        priority >= LOW_PRIORITY && priority <= HIGH_PRIORITY,
        priority,
        "librertos_create_task(): invalid priority.");

    scheduler_lock();
    CRITICAL_ENTER();

    /* Make non-zero, to be easy to spot uninitialized fields. */
    memset(task, NONZERO_INITVAL, sizeof(*task));

    task->func = func;
    task->param = param;
    task->task_state = TASK_NOT_RUNNING;
    task->priority = priority;
    task->original_priority = priority;
    task->delay_until = 0;
    node_init(&task->sched_node, task);
    node_init(&task->event_node, task);

    list_insert_last(&librertos.tasks_ready[priority], &task->sched_node);

    CRITICAL_EXIT();
    scheduler_unlock();
}

/*
 * Start LibreRTOS, allows the scheduler to run on interrupts and when resuming
 * tasks.
 *
 * Call after initializing the hardware, tasks, but before calling the scheduler
 * in a loop.
 */
void librertos_start(void) {
    --librertos.scheduler_lock;
}

static task_t *get_higher_priority_task(task_t *current_task) {
    struct node_t *node;
    task_t *task;
    int8_t current_priority = (current_task == NO_TASK_PTR)
                                  ? NO_TASK_PRIORITY
                                  : current_task->priority;
    int8_t i;

    for (i = HIGH_PRIORITY; i > current_priority; i--) {
        if (list_is_empty(&librertos.tasks_ready[i]))
            continue;

        node = list_get_first(&librertos.tasks_ready[i]);
        task = (task_t *)node->owner;

        if (task->task_state == TASK_RUNNING)
            break;

        list_remove(node);
        list_insert_last(&librertos.tasks_ready[i], node);

        return task;
    }

    return NULL;
}

/*
 * Run scheduled tasks.
 *
 * Picks and runs the higher priority task that is ready.
 */
void librertos_sched(void) {
    task_t *current_task;
    INTERRUPTS_VAL();

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

    while (1) {
        task_t *task = get_higher_priority_task(current_task);
        if (task == NULL)
            break;

        librertos.current_task = task;
        task->task_state = TASK_RUNNING;

        /* Enable interrupts while running the task. */
        INTERRUPTS_ENABLE();
        task->func(task->param);
        INTERRUPTS_DISABLE();

        task->task_state = TASK_NOT_RUNNING;
    }

    librertos.current_task = current_task;
    librertos.scheduler_depth--;
    INTERRUPTS_ENABLE();
}

/*
 * Lock scheduler, avoids preemption when using the preemptive kernel.
 */
void scheduler_lock(void) {
    if (KERNEL_MODE == LIBRERTOS_PREEMPTIVE) {
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
void scheduler_unlock(void) {
    if (KERNEL_MODE == LIBRERTOS_PREEMPTIVE) {
        CRITICAL_VAL();

        CRITICAL_ENTER();

        if (--librertos.scheduler_lock == 0) {
            CRITICAL_EXIT();
            librertos_sched();
        } else {
            CRITICAL_EXIT();
        }
    }
}

/*
 * Lock scheduler for interrupts, avoids preemption when using the preemptive
 * kernel. Returns the current task so it can be restored when unlocking the
 * scheduler.
 *
 * This function must be called in the begining of interrupts that use LibreRTOS
 * functions.
 *
 * @return Task preempted by the interrupt.
 */
task_t *interrupt_lock(void) {
    task_t *task;

    scheduler_lock();
    task = get_current_task();
    set_current_task(INTERRUPT_TASK_PTR);

    return task;
}

/*
 * Unlock scheduler for interrupts, causes preemption when using the preemptive
 * kernel and a higher priority tasks is ready. Pass the task returned by
 * interrupt_lock().
 *
 * This function must be called in the end of interrupts that use LibreRTOS
 * functions.
 *
 * @param task Task preempted by the interrupt.
 */
void interrupt_unlock(task_t *task) {
    set_current_task(task);
    scheduler_unlock();
}

static void resume_delayed_tasks(tick_t now);

/*
 * Process a tick timer interrupt. Increment the tick counter and resume
 * suspended tasks.
 *
 * Must be called periodically by a timer interrupt.
 */
void librertos_tick_interrupt(void) {
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
tick_t get_tick(void) {
    tick_t tick;
    CRITICAL_VAL();

    CRITICAL_ENTER();

    tick = librertos.tick;

    CRITICAL_EXIT();

    return tick;
}

/*
 * Get the currently running task, NO_TASK_PTR (a.k.a NULL) if no task is
 * running and INTERRUPT_TASK_PTR if an interrupt is running.
 */
task_t *get_current_task(void) {
    task_t *task;
    CRITICAL_VAL();

    CRITICAL_ENTER();

    task = librertos.current_task;

    CRITICAL_EXIT();

    return task;
}

/*
 * Set the currently running task.
 */
void set_current_task(task_t *task) {
    CRITICAL_VAL();

    CRITICAL_ENTER();

    librertos.current_task = task;

    CRITICAL_EXIT();
}

/* Call with interrupts disabled and scheduler locked. */
static void swap_lists_of_delayed_tasks(void) {
    struct list_t *temp = librertos.tasks_delayed_overflow;
    librertos.tasks_delayed_overflow = librertos.tasks_delayed_current;
    librertos.tasks_delayed_current = temp;
}

/* Call with interrupts disabled and scheduler locked. */
static void resume_list_of_tasks(struct list_t *list, tick_t now) {
    INTERRUPTS_VAL();

    while (list->length != 0) {
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

/* Call with interrupts disabled and scheduler locked. */
static void resume_delayed_tasks(tick_t now) {
    if (now == 0) {
        /* Tick overflow. */
        swap_lists_of_delayed_tasks();
        resume_list_of_tasks(librertos.tasks_delayed_overflow, MAX_DELAY);
    }

    resume_list_of_tasks(librertos.tasks_delayed_current, now);
}

/* Call with interrupts disabled and scheduler locked. */
static struct node_t *delay_find_tick_position(struct list_t *list, tick_t tick) {
    struct node_t *head = LIST_HEAD(list);
    struct node_t *pos;
    INTERRUPTS_VAL();

    pos = list->head;

    while (pos != head) {
        task_t *task = (task_t *)pos->owner;
        tick_t pos_tick = task->delay_until;

        INTERRUPTS_ENABLE();

        /* Compare outside of critical section. */
        if (tick < pos_tick) {
            /* Found the position: before pos. Stop. */
            INTERRUPTS_DISABLE();
            break;
        }

        INTERRUPTS_DISABLE();

        if (pos->list != list) {
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

/* No restrictions when calling. */
static void task_delay_now_until(tick_t now, tick_t tick_to_wakeup) {
    task_t *task;
    struct node_t *node;
    struct node_t *pos;
    struct list_t *delay_list;
    CRITICAL_VAL();

    (void)tick_to_wakeup;

    scheduler_lock();
    CRITICAL_ENTER();

    task = librertos.current_task;
    node = &task->sched_node;
    task->delay_until = tick_to_wakeup;
    delay_list = (now < tick_to_wakeup) ? librertos.tasks_delayed_current
                                        : librertos.tasks_delayed_overflow;

    /* Suspend the task so that it can be resumed. */
    task_suspend(task);

    do {
        pos = delay_find_tick_position(delay_list, tick_to_wakeup);

        /* Check if pos was removed from the list during the comparison or
         * return. Restart. */
    } while (pos != LIST_HEAD(delay_list) && pos->list != delay_list);

    /* Check if current task was not resumed. */
    if (node->list == &librertos.tasks_suspended) {
        /* Update the position. */
        list_remove(node);
        list_insert_after(delay_list, pos, node);
    }

    CRITICAL_EXIT();
    scheduler_unlock();
}

/*
 * Delay task for a certain amount of ticks.
 *
 * This function can be used only by tasks.
 *
 * If the task is currently running, it will keep running until it returns.
 */
void task_delay(tick_t ticks_to_delay) {
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
void task_suspend(task_t *task) {
    CRITICAL_VAL();

    LIBRERTOS_ASSERT(
        !(task == CURRENT_TASK_PTR && (librertos.current_task == NO_TASK_PTR || librertos.current_task == INTERRUPT_TASK_PTR)),
        (intptr_t)librertos.current_task,
        "task_suspend(): no task or interrupt is running.");

    CRITICAL_ENTER();

    if (task == CURRENT_TASK_PTR)
        task = librertos.current_task;

    list_remove(&task->sched_node);
    list_insert_first(&librertos.tasks_suspended, &task->sched_node);

    CRITICAL_EXIT();
}

/*
 * Resume task.
 */
void task_resume(task_t *task) {
    struct list_t *list_ready;
    struct node_t *node_sched;
    struct node_t *node_event;
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
void task_resume_all(void) {
    CRITICAL_VAL();

    scheduler_lock();
    CRITICAL_ENTER();

    resume_list_of_tasks(&librertos.tasks_suspended, MAX_DELAY);
    resume_list_of_tasks(&librertos.tasks_delayed[0], MAX_DELAY);
    resume_list_of_tasks(&librertos.tasks_delayed[1], MAX_DELAY);

    CRITICAL_EXIT();
    scheduler_unlock();
}

/* Call with interrupts disabled. */
void list_init(struct list_t *list) {
    list->head = (struct node_t *)list;
    list->tail = (struct node_t *)list;
    list->length = 0;
}

/* Call with interrupts disabled. */
void node_init(struct node_t *node, void *owner) {
    node->next = NULL;
    node->prev = NULL;
    node->list = NULL;
    node->owner = owner;
}

/* Call with interrupts disabled. */
uint8_t node_in_list(struct node_t *node) {
    return node->list != NULL;
}

/* Call with interrupts disabled. */
void list_insert_after(
    struct list_t *list, struct node_t *pos, struct node_t *node) {
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

/* Call with interrupts disabled. */
void list_insert_first(struct list_t *list, struct node_t *node) {
    list_insert_after(list, LIST_HEAD(list), node);
}

/* Call with interrupts disabled. */
void list_insert_last(struct list_t *list, struct node_t *node) {
    list_insert_after(list, list->tail, node);
}

/* Call with interrupts disabled. */
void list_remove(struct node_t *node) {
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

/* Call with interrupts disabled. */
struct node_t *list_get_first(struct list_t *list) {
    return list->head;
}

/* Call with interrupts disabled. */
uint8_t list_is_empty(struct list_t *list) {
    return list->length == 0;
}

/* Call with interrupts disabled. */
void event_init(event_t *event) {
    list_init(&event->suspended_tasks);
}

/* Call with interrupts disabled and scheduler locked. */
static struct node_t *
event_find_priority_position(struct list_t *list, int8_t priority) {
    struct node_t *head = LIST_HEAD(list);
    struct node_t *pos;
    INTERRUPTS_VAL();

    pos = list->head;

    while (pos != head) {
        task_t *task = (task_t *)pos->owner;
        int8_t pos_priority = task->priority;

        INTERRUPTS_ENABLE();

        /* Compare outside of critical section. */
        if (priority > pos_priority) {
            /* Found the position: before pos. Stop. */
            INTERRUPTS_DISABLE();
            break;
        }

        INTERRUPTS_DISABLE();

        if (pos->list != list) {
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

/* Call with interrupts disabled and scheduler locked. */
void event_add_task_to_event(event_t *event) {
    task_t *task = librertos.current_task;
    int8_t task_priority = task->priority;
    struct node_t *pos;

    list_insert_last(&event->suspended_tasks, &task->event_node);

    do {
        pos = event_find_priority_position(
            &event->suspended_tasks, task_priority);

        /* Check if pos was removed from the list during the comparison or
         * return. Restart. */
    } while (pos != LIST_HEAD(&event->suspended_tasks) && pos->list != &event->suspended_tasks);

    /* Check if current task was not resumed. */
    if (task->sched_node.list == &librertos.tasks_suspended) {
        if (pos == &task->event_node) {
            /* Already in the correct position. */
        } else {
            /* Update the position. */
            list_remove(&task->event_node);
            list_insert_after(&event->suspended_tasks, pos, &task->event_node);
        }
    }
}

/* Call with interrupts disabled and scheduler locked. */
void event_delay_task(event_t *event, tick_t ticks_to_delay) {
    LIBRERTOS_ASSERT(
        !(librertos.current_task == NO_TASK_PTR || librertos.current_task == INTERRUPT_TASK_PTR),
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

    if (ticks_to_delay != MAX_DELAY) {
        tick_t now = get_tick();
        tick_t tick_to_wakeup = now + ticks_to_delay;
        task_delay_now_until(now, tick_to_wakeup);
    }
}

/* Call with interrupts disabled. */
void event_resume_task(event_t *event) {
    if (event->suspended_tasks.length != 0) {
        task_t *task = (task_t *)list_get_first(&event->suspended_tasks)->owner;
        task_resume(task);
    }
}

/*
 * Initialize the semaphore with an initial value and a maximum value.
 *
 * It is recommended to use the functions below instead of this:
 * - semaphore_init_locked()
 * - semaphore_init_unlocked()
 *
 * @param init_count Initial value of the semaphore.
 * @param max_count Maximum value of the semaphore.
 */
void semaphore_init(semaphore_t *sem, uint8_t init_count, uint8_t max_count) {
    CRITICAL_VAL();

    LIBRERTOS_ASSERT(
        init_count <= max_count,
        init_count,
        "semaphore_init(): invalid init_count.");

    CRITICAL_ENTER();

    /* Make non-zero, to be easy to spot uninitialized fields. */
    memset(sem, NONZERO_INITVAL, sizeof(*sem));

    sem->count = init_count;
    sem->max = max_count;
    event_init(&sem->event_unlock);

    CRITICAL_EXIT();
}

/*
 * Initialize the semaphore in the locked state (initial value equals zero).
 *
 * @param max_count Maximum value of the semaphore.
 */
void semaphore_init_locked(semaphore_t *sem, uint8_t max_count) {
    semaphore_init(sem, 0, max_count);
}

/*
 * Initialize semaphore in the unlocked state (initial value equals maximum
 * value).
 *
 * @param max_count Maximum value of the semaphore.
 */
void semaphore_init_unlocked(semaphore_t *sem, uint8_t max_count) {
    semaphore_init(sem, max_count, max_count);
}

/* Call with interrupts disabled. */
static uint8_t semaphore_can_be_locked(semaphore_t *sem) {
    return sem->count > 0;
}

/* Call with interrupts disabled. */
static uint8_t semaphore_can_be_unlocked(semaphore_t *sem) {
    return sem->count < sem->max;
}

/*
 * Lock the semaphore.
 *
 * @return 1 with success, 0 otherwise.
 */
result_t semaphore_lock(semaphore_t *sem) {
    result_t result = FAIL;
    CRITICAL_VAL();

    CRITICAL_ENTER();

    if (semaphore_can_be_locked(sem)) {
        sem->count--;
        result = SUCCESS;
    }

    CRITICAL_EXIT();

    return result;
}

/*
 * Unlock the semaphore.
 *
 * @return 1 with success, 0 otherwise.
 */
result_t semaphore_unlock(semaphore_t *sem) {
    result_t result = FAIL;
    CRITICAL_VAL();

    CRITICAL_ENTER();

    if (semaphore_can_be_unlocked(sem)) {
        scheduler_lock();

        sem->count++;
        result = SUCCESS;

        event_resume_task(&sem->event_unlock);

        CRITICAL_EXIT();
        scheduler_unlock();
    } else {
        CRITICAL_EXIT();
    }

    return result;
}

/*
 * Get the current value of the semaphore.
 */
uint8_t semaphore_get_count(semaphore_t *sem) {
    uint8_t count;
    CRITICAL_VAL();

    CRITICAL_ENTER();

    count = sem->count;

    CRITICAL_EXIT();

    return count;
}

/*
 * Get the maximum value of the semaphore.
 */
uint8_t semaphore_get_max(semaphore_t *sem) {
    /* Semaphore maximum value should not change.
     * Critical section is not necessary. */
    return sem->max;
}

/*
 * Suspend the task on the semaphore, waiting a maximum number for ticks to pass
 * before resuming.
 *
 * This function can be used only by tasks.
 *
 * @param ticks_to_delay Number of ticks to delay resuming the task. Pass
 * MAX_DELAY to wait forever.
 */
void semaphore_suspend(semaphore_t *sem, tick_t ticks_to_delay) {
    CRITICAL_VAL();

    CRITICAL_ENTER();

    if (!semaphore_can_be_locked(sem)) {
        scheduler_lock();

        event_delay_task(&sem->event_unlock, ticks_to_delay);

        CRITICAL_EXIT();
        scheduler_unlock();
    } else {
        CRITICAL_EXIT();
    }
}

/*
 * Lock the semaphore if unlocked, else tries to suspend the task on the
 * semaphore, waiting a maximum number for ticks to pass before resuming.
 *
 * This function can be used only by tasks.
 *
 * @param ticks_to_delay Number of ticks to delay resuming the task. Pass
 * MAX_DELAY to wait forever.
 * @return 1 with success locking, 0 otherwise.
 */
result_t semaphore_lock_suspend(semaphore_t *sem, tick_t ticks_to_delay) {
    result_t result;

    result = semaphore_lock(sem);
    if (result == FAIL)
        semaphore_suspend(sem, ticks_to_delay);

    return result;
}

/*
 * Initialize mutex.
 */
void mutex_init(mutex_t *mtx) {
    CRITICAL_VAL();

    CRITICAL_ENTER();

    /* Make non-zero, to be easy to spot uninitialized fields. */
    memset(mtx, NONZERO_INITVAL, sizeof(*mtx));

    mtx->locked = 0;
    mtx->task_owner = NO_TASK_PTR;
    event_init(&mtx->event_unlock);

    CRITICAL_EXIT();
}

/* Call with interrupts disabled. */
static uint8_t mutex_can_be_locked(mutex_t *mtx, task_t *current_task) {
    return mtx->locked == MUTEX_UNLOCKED || current_task == mtx->task_owner;
}

/*
 * Lock the mutex.
 *
 * @return 1 with success, 0 otherwise.
 */
result_t mutex_lock(mutex_t *mtx) {
    result_t result = FAIL;
    task_t *current_task;
    CRITICAL_VAL();

    CRITICAL_ENTER();

    current_task = librertos.current_task;

    if (mutex_can_be_locked(mtx, current_task)) {
        ++(mtx->locked);
        mtx->task_owner = current_task;
        result = SUCCESS;
    }

    CRITICAL_EXIT();

    return result;
}

/* Call with interrupts disabled. */
static void task_set_priority(task_t *task, int8_t priority) {
    struct list_t *ready_list = &librertos.tasks_ready[task->priority];
    event_t *event_list = (event_t *)task->event_node.list;

    task->priority = priority;

    if (task->sched_node.list == ready_list) {
        /* Put in the correct ready list. */
        list_remove(&task->sched_node);
        list_insert_first(&librertos.tasks_ready[priority], &task->sched_node);
    }

    if (event_list != NULL) {
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
 * Unlock the mutex.
 */
void mutex_unlock(mutex_t *mtx) {
    CRITICAL_VAL();

    LIBRERTOS_ASSERT(
        mtx->locked != MUTEX_UNLOCKED,
        mtx->locked,
        "mutex_unlock(): mutex already unlocked.");

    CRITICAL_ENTER();

    if (mtx->locked != MUTEX_UNLOCKED) {
        --(mtx->locked);
    }

    if (mtx->locked == MUTEX_UNLOCKED) {
        task_t *owner;

        scheduler_lock();

        owner = (task_t *)mtx->task_owner;

        if (owner == NO_TASK_PTR || owner == INTERRUPT_TASK_PTR) {
            /* Cannot change priority of no task or interrupt. */
        } else {
            if (owner->priority != owner->original_priority)
                task_set_priority(owner, owner->original_priority);
            mtx->task_owner = NO_TASK_PTR;
        }

        event_resume_task(&mtx->event_unlock);
        CRITICAL_EXIT();
        scheduler_unlock();
    } else {
        CRITICAL_EXIT();
    }
}

/*
 * Check if the mutex is locked.
 */
uint8_t mutex_is_locked(mutex_t *mtx) {
    uint8_t locked;
    CRITICAL_VAL();

    CRITICAL_ENTER();

    locked = !mutex_can_be_locked(mtx, librertos.current_task);

    CRITICAL_EXIT();

    return locked;
}

/*
 * Suspend the task on the mutex, waiting a maximum number for ticks to pass
 * before resuming.
 *
 * This function can be used only by tasks.
 *
 * @param ticks_to_delay Number of ticks to delay resuming the task. Pass
 * MAX_DELAY to wait forever.
 */
void mutex_suspend(mutex_t *mtx, tick_t ticks_to_delay) {
    CRITICAL_VAL();

    CRITICAL_ENTER();

    if (!mutex_can_be_locked(mtx, librertos.current_task)) {
        task_t *owner = (task_t *)mtx->task_owner;
        int8_t current_priority = librertos.current_task->priority;

        scheduler_lock();

        if (owner == NO_TASK_PTR || owner == INTERRUPT_TASK_PTR) {
            /* Cannot change priority of no task or interrupt. */
        } else {
            if (owner->priority < current_priority)
                task_set_priority(owner, current_priority);
        }

        event_delay_task(&mtx->event_unlock, ticks_to_delay);

        CRITICAL_EXIT();
        scheduler_unlock();
    } else {
        CRITICAL_EXIT();
    }
}

/*
 * Lock the mutex if unlocked, else tries to suspend the task on the
 * mutex, waiting a maximum number for ticks to pass before resuming.
 *
 * This function can be used only by tasks.
 *
 * @param ticks_to_delay Number of ticks to delay resuming the task. Pass
 * MAX_DELAY to wait forever.
 * @return 1 with success locking, 0 otherwise.
 */
result_t mutex_lock_suspend(mutex_t *mtx, tick_t ticks_to_delay) {
    result_t result;

    result = mutex_lock(mtx);
    if (result == FAIL)
        mutex_suspend(mtx, ticks_to_delay);

    return result;
}

/*
 * Initialize the queue.
 *
 * @param buff Pointer to a buffer with size que_size*item_size.
 * @param que_size Number of items the queue can hold.
 * @param item_size Size of the items in the queue.
 */
void queue_init(queue_t *que, void *buff, uint8_t que_size, uint8_t item_size) {
    CRITICAL_VAL();

    CRITICAL_ENTER();

    /* Make non-zero, to be easy to spot uninitialized fields. */
    memset(que, NONZERO_INITVAL, sizeof(*que));

    que->free = que_size;
    que->used = 0;
    que->head = 0;
    que->tail = 0;
    que->item_size = item_size;
    que->end = que_size * item_size;
    que->buff = (uint8_t *)buff;
    event_init(&que->event_write);

    CRITICAL_EXIT();
}

/* Call with interrupts disabled. */
static uint8_t queue_can_be_read(queue_t *que) {
    return que->used > 0;
}

/* Call with interrupts disabled. */
static uint8_t queue_can_be_written(queue_t *que) {
    return que->free > 0;
}

/*
 * Read an item from the queue.
 *
 * @param data Pointer to a buffer with size que->item_size.
 * @return 1 with success, 0 otherwise.
 */
result_t queue_read(queue_t *que, void *data) {
    result_t result = FAIL;
    CRITICAL_VAL();

    CRITICAL_ENTER();

    if (queue_can_be_read(que)) {
        memcpy(data, &que->buff[que->tail], que->item_size);

        que->tail += que->item_size;
        if (que->tail >= que->end)
            que->tail = 0;

        que->free++;
        que->used--;
        result = SUCCESS;
    }

    CRITICAL_EXIT();

    return result;
}

/*
 * Write an item to the queue.
 *
 * @param data Pointer to a buffer with size que->item_size.
 * @return 1 with success, 0 otherwise.
 */
result_t queue_write(queue_t *que, const void *data) {
    result_t result = FAIL;
    CRITICAL_VAL();

    CRITICAL_ENTER();

    if (queue_can_be_written(que)) {
        scheduler_lock();

        memcpy(&que->buff[que->head], data, que->item_size);

        que->head += que->item_size;
        if (que->head >= que->end)
            que->head = 0;

        que->free--;
        que->used++;
        result = SUCCESS;

        event_resume_task(&que->event_write);

        CRITICAL_EXIT();
        scheduler_unlock();
    } else {
        CRITICAL_EXIT();
    }

    return result;
}

/*
 * Get number of free items in the queue.
 */
uint8_t queue_get_num_free(queue_t *que) {
    uint8_t value;
    CRITICAL_VAL();

    CRITICAL_ENTER();

    value = que->free;

    CRITICAL_EXIT();

    return value;
}

/*
 * Get number of used items in the queue.
 */
uint8_t queue_get_num_used(queue_t *que) {
    uint8_t value;
    CRITICAL_VAL();

    CRITICAL_ENTER();

    value = que->used;

    CRITICAL_EXIT();

    return value;
}

/*
 * Check if the queue is empty (zero items used).
 */
uint8_t queue_is_empty(queue_t *que) {
    return queue_get_num_used(que) == 0;
}

/*
 * Check if the queue is full (all items used).
 */
uint8_t queue_is_full(queue_t *que) {
    return queue_get_num_free(que) == 0;
}

/*
 * Get total number of items in the queue (free + used).
 */
uint8_t queue_get_num_items(queue_t *que) {
    uint8_t value;
    CRITICAL_VAL();

    CRITICAL_ENTER();

    value = que->free + que->used;

    CRITICAL_EXIT();

    return value;
}

/*
 * Get the size of the items in the queue.
 */
uint8_t queue_get_item_size(queue_t *que) {
    /* Queue item size should not change.
     * Critical section is not necessary. */
    return que->item_size;
}

/*
 * Suspend the task waiting to read the queue, waiting a maximum number for
 * ticks to pass before resuming.
 *
 * This function can be used only by tasks.
 *
 * @param ticks_to_delay Number of ticks to delay resuming the task. Pass
 * MAX_DELAY to wait forever.
 */
void queue_suspend(queue_t *que, tick_t ticks_to_delay) {
    CRITICAL_VAL();

    CRITICAL_ENTER();

    if (!queue_can_be_read(que)) {
        scheduler_lock();

        event_delay_task(&que->event_write, ticks_to_delay);

        CRITICAL_EXIT();
        scheduler_unlock();
    } else {
        CRITICAL_EXIT();
    }
}

/*
 * Read an item from the the queue if not empty, else tries to suspend the task
 * waiting to read the queue, waiting a maximum number for ticks to pass before
 * resuming.
 *
 * This function can be used only by tasks.
 *
 * @param data Pointer to a buffer with size que->item_size.
 * @param ticks_to_delay Number of ticks to delay resuming the task. Pass
 * MAX_DELAY to wait forever.
 * @return 1 with success reading, 0 otherwise.
 */
result_t queue_read_suspend(queue_t *que, void *data, tick_t ticks_to_delay) {
    result_t result;

    result = queue_read(que, data);
    if (result == FAIL)
        queue_suspend(que, ticks_to_delay);

    return result;
}
