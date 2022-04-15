/* Copyright (c) 2022 Djones A. Boni - MIT License */

#ifndef LIBRERTOS_H_
#define LIBRERTOS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "librertos_port.h"
#include "librertos_proj.h"
#include <stdint.h>

#define NO_TASK_PTR ((task_t *)0)
#define CURRENT_TASK_PTR ((task_t *)0)
#define INTERRUPT_TASK_PTR ((task_t *)1)

#define PERIODIC(delay_ticks, code) \
    do \
    { \
        const difftick_t __delay = (delay_ticks); \
        static tick_t __last = -__delay; \
        tick_t __now = get_tick(); \
        difftick_t __diff = __now - __last; \
        if (__diff >= __delay) \
        { \
            __last += __delay; \
            { \
                code; \
            } \
        } \
    } while (0)

struct os_task_t;
struct node_t;

typedef enum
{
    FAIL = 0,
    SUCCESS = 1
} result_t;

typedef enum
{
    LOW_PRIORITY = 0,
    HIGH_PRIORITY = NUM_PRIORITIES - 1
} priority_t;

typedef enum
{
    LIBRERTOS_PREEMPTIVE = 0,
    LIBRERTOS_COOPERATIVE
} kernel_mode_t;

struct list_t
{
    struct node_t *head;
    struct node_t *tail;
    uint8_t length;
};

struct node_t
{
    struct node_t *next;
    struct node_t *prev;
    struct list_t *list;
    void *owner;
};

typedef struct
{
    struct list_t suspended_tasks;
} event_t;

typedef struct
{
    uint8_t count;
    uint8_t max;
    event_t event_unlock;
} semaphore_t;

typedef struct
{
    uint8_t locked;
    struct os_task_t *task_owner;
    event_t event_unlock;
} mutex_t;

typedef struct
{
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

typedef struct os_task_t
{
    task_function_t func;
    task_parameter_t param;
    int8_t priority;
    int8_t original_priority;
    struct node_t sched_node;
    struct node_t event_node;
} task_t;

typedef struct
{
    int8_t scheduler_lock;
    tick_t tick;
    task_t *current_task;
    struct list_t tasks_ready[NUM_PRIORITIES];
    struct list_t tasks_suspended;
} librertos_t;

void semaphore_init(semaphore_t *sem, uint8_t init_count, uint8_t max_count);
void semaphore_init_locked(semaphore_t *sem, uint8_t max_count);
void semaphore_init_unlocked(semaphore_t *sem, uint8_t max_count);
result_t semaphore_lock(semaphore_t *sem);
result_t semaphore_unlock(semaphore_t *sem);
uint8_t semaphore_get_count(semaphore_t *sem);
uint8_t semaphore_get_max(semaphore_t *sem);
void semaphore_suspend(semaphore_t *sem);
result_t semaphore_lock_suspend(semaphore_t *sem);

void mutex_init(mutex_t *mtx);
result_t mutex_lock(mutex_t *mtx);
void mutex_unlock(mutex_t *mtx);
uint8_t mutex_is_locked(mutex_t *mtx);
void mutex_suspend(mutex_t *mtx);
result_t mutex_lock_suspend(mutex_t *mtx);

void queue_init(queue_t *que, void *buff, uint8_t que_size, uint8_t item_size);
result_t queue_read(queue_t *que, void *data);
result_t queue_write(queue_t *que, const void *data);
uint8_t queue_get_num_free(queue_t *que);
uint8_t queue_get_num_used(queue_t *que);
uint8_t queue_is_empty(queue_t *que);
uint8_t queue_is_full(queue_t *que);
uint8_t queue_get_num_items(queue_t *que);
uint8_t queue_get_item_size(queue_t *que);
void queue_suspend(queue_t *que);
result_t queue_read_suspend(queue_t *que, void *data);

void librertos_init(void);
void librertos_tick_interrupt(void);
void librertos_create_task(
    int8_t priority,
    task_t *task,
    task_function_t func,
    task_parameter_t param);
void librertos_start(void);
void librertos_sched(void);

void scheduler_lock(void);
void scheduler_unlock(void);
task_t *interrupt_lock(void);
void interrupt_unlock(task_t *task);

tick_t get_tick(void);
task_t *get_current_task(void);
void task_suspend(task_t *task);
void task_resume(task_t *task);

#ifdef __cplusplus
}
#endif

#endif /* LIBRERTOS_H_ */
