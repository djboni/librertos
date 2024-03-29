/* Copyright (c) 2016-2023 Djones A. Boni - MIT License */

#ifndef LIBRERTOS_H_
#define LIBRERTOS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "librertos_proj.h"

#include "librertos_port.h"
#include <stdint.h>

#define MAX_DELAY ((tick_t)-1)

struct os_task_t;
struct node_t;

typedef enum {
    LIBRERTOS_FAIL = 0,
    LIBRERTOS_SUCCESS = 1
} result_t;

typedef enum {
    LOW_PRIORITY = 0,
    HIGH_PRIORITY = NUM_PRIORITIES - 1
} priority_t;

typedef enum {
    LIBRERTOS_PREEMPTIVE = 0,
    LIBRERTOS_COOPERATIVE
} kernel_mode_t;

typedef enum {
    TIMERTYPE_AUTO = 1, /* Auto reset timer after it has run. */
    TIMERTYPE_ONESHOT   /* Timer need to be reset to run. */
} timer_type_t;

struct list_t {
    struct node_t *head;
    struct node_t *tail;
    uint8_t length;
};

struct node_t {
    struct node_t *next;
    struct node_t *prev;
    struct list_t *list;
    void *owner;
};

typedef struct {
    struct list_t suspended_tasks;
} event_t;

typedef struct {
    uint8_t count;
    uint8_t max;
    event_t event_unlock;
} semaphore_t;

typedef struct {
    uint8_t count;
    struct os_task_t *task_owner;
    event_t event_unlock;
} mutex_t;

typedef struct {
    uint8_t free;
    uint8_t used;
    uint16_t head;
    uint16_t tail;
    uint16_t item_size;
    uint16_t end;
    uint8_t *buff;
    event_t event_write;
} queue_t;

typedef void *task_parameter_t;
typedef void (*task_function_t)(task_parameter_t param);

typedef struct os_task_t {
    task_function_t func;
    task_parameter_t param;
    int8_t task_state;
    int8_t priority;
    int8_t original_priority;
    tick_t delay_until;
    struct node_t sched_node;
    struct node_t event_node;
} task_t;

struct timer_task_t;
typedef void *timer_parameter_t;
typedef void (*timer_function_t)(struct timer_task_t *timer, timer_parameter_t param);

typedef struct timer_task_t {
    timer_type_t type;
    tick_t period;
    timer_function_t func;
    timer_parameter_t param;
    task_t timer_task;
} timer_task_t;

void librertos_init(void);
void librertos_start(void);
void librertos_sched(void);

void scheduler_lock(void);
void scheduler_unlock(void);
task_t *interrupt_lock(void);
void interrupt_unlock(task_t *interrupted_task);

void librertos_tick_interrupt(void);
tick_t get_tick(void);

void librertos_create_task(int8_t priority, task_t *task,
    task_function_t func, task_parameter_t param);
void task_delay(tick_t ticks_to_delay);
void task_suspend(task_t *task);
void task_resume(task_t *task);
void task_resume_all(void);

void librertos_create_timer(int8_t priority, timer_task_t *timer,
    timer_function_t func, timer_parameter_t param,
    timer_type_t timer_type, tick_t timer_period);
void timer_start(timer_task_t *timer);
void timer_reset(timer_task_t *timer);
void timer_stop(timer_task_t *timer);

void semaphore_init(semaphore_t *sem, uint8_t init_count, uint8_t max_count);
void semaphore_init_locked(semaphore_t *sem, uint8_t max_count);
void semaphore_init_unlocked(semaphore_t *sem, uint8_t max_count);
result_t semaphore_lock(semaphore_t *sem);
result_t semaphore_unlock(semaphore_t *sem);
uint8_t semaphore_get_count(semaphore_t *sem);
uint8_t semaphore_get_max(semaphore_t *sem);
void semaphore_suspend(semaphore_t *sem, tick_t ticks_to_delay);
result_t semaphore_lock_suspend(semaphore_t *sem, tick_t ticks_to_delay);

void mutex_init(mutex_t *mtx);
result_t mutex_lock(mutex_t *mtx);
void mutex_unlock(mutex_t *mtx);
uint8_t mutex_is_locked(mutex_t *mtx);
void mutex_suspend(mutex_t *mtx, tick_t ticks_to_delay);
result_t mutex_lock_suspend(mutex_t *mtx, tick_t ticks_to_delay);

void queue_init(queue_t *que, void *buff, uint8_t que_size, uint8_t item_size);
result_t queue_read(queue_t *que, void *data);
result_t queue_write(queue_t *que, const void *data);
uint8_t queue_is_empty(queue_t *que);
uint8_t queue_is_full(queue_t *que);
uint8_t queue_get_num_free(queue_t *que);
uint8_t queue_get_num_used(queue_t *que);
uint8_t queue_get_num_items(queue_t *que);
uint8_t queue_get_item_size(queue_t *que);
void queue_suspend(queue_t *que, tick_t ticks_to_delay);
result_t queue_read_suspend(queue_t *que, void *data, tick_t ticks_to_delay);

/**
 * Run block periodically, every 'delay_ticks' ticks.
 *
 * This is useful to run non-time-critical procedures in a low priority
 * task, such as the IDLE task.
 *
 * With C90 (the oldest C standard) this macro may
 * generate warnings about definitions after statements, which you can avoid
 * by placing the macro in a block. See the examples below.
 *
 * Example:
 *
 * ```cpp
 * PERIODIC_BLOCK(0.5 * TICKS_PER_SECOND) {
 *     printf("tick=%u\n", get_tick());
 * }
 * ```
 *
 * Example with C90:
 *
 * ```cpp
 * {
 *     // In a block to avoid C90 warning.
 *
 *     PERIODIC_BLOCK(0.5 * TICKS_PER_SECOND) {
 *         printf("tick=%u\n", get_tick());
 *     }
 * }
 * ```
 */
#define PERIODIC_BLOCK(delay_ticks) \
    static tick_t __last_##__LINE__; \
    tick_t __now; \
    for (__now = get_tick(); \
         __now - __last_##__LINE__ >= (delay_ticks); \
         __last_##__LINE__ = __now)

/*
 * Define LIBRERTOS_DEBUG_DECLARATIONS before including this header to
 * make these definitions available.
 */
#ifdef LIBRERTOS_DEBUG_DECLARATIONS

typedef struct {
    int8_t scheduler_depth;
    int8_t higher_priority_task_ready;
    tick_t tick;
    task_t *current_task;
    struct list_t tasks_ready[NUM_PRIORITIES];
    struct list_t tasks_suspended;
    struct list_t *tasks_delayed_current;
    struct list_t *tasks_delayed_overflow;
    struct list_t tasks_delayed[2];
} librertos_t;

extern librertos_t librertos;

task_t *get_current_task(void);
void set_current_task(task_t *task);

void list_init(struct list_t *list);
void node_init(struct node_t *node, void *owner);
uint8_t node_in_list(struct node_t *node);
void list_insert_first(struct list_t *list, struct node_t *node);
void list_insert_last(struct list_t *list, struct node_t *node);
void list_insert_after(struct list_t *list, struct node_t *pos, struct node_t *node);
void list_remove(struct node_t *node);
struct node_t *list_get_first(struct list_t *list);
uint8_t list_is_empty(struct list_t *list);

void event_init(event_t *event);
void event_add_task_to_event(event_t *event);
void event_delay_task(event_t *event, tick_t ticks_to_delay);
void event_resume_task(event_t *event);

#endif /* LIBRERTOS_DEBUG_DECLARATIONS */

#ifdef __cplusplus
}
#endif

#endif /* LIBRERTOS_H_ */
