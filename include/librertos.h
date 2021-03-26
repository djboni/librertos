/*
 LibreRTOS - Portable single-stack Real Time Operating System.

 Copyright 2016-2021 Djones A. Boni

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#ifndef LIBRERTOS_H_
#define LIBRERTOS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "projdefs.h"
#include <stdint.h>

#ifndef LIBRERTOS_MAX_PRIORITY
#define LIBRERTOS_MAX_PRIORITY 3 /* integer > 0 */
#endif

#ifndef LIBRERTOS_PREEMPTION
#define LIBRERTOS_PREEMPTION 0 /* boolean */
#endif

#ifndef LIBRERTOS_PREEMPT_LIMIT
#define LIBRERTOS_PREEMPT_LIMIT 0 /* integer >= 0, < LIBRERTOS_MAX_PRIORITY */
#endif

#if (LIBRERTOS_PREEMPT_LIMIT < 0)
#error "LIBRERTOS_PREEMPT_LIMIT < 0! Should be >= 0."
#endif

#if (LIBRERTOS_PREEMPT_LIMIT >= LIBRERTOS_MAX_PRIORITY)
#error                                                                         \
    "LIBRERTOS_PREEMPT_LIMIT >= LIBRERTOS_MAX_PRIORITY! Makes the kernel cooperative! Should be < LIBRERTOS_MAX_PRIORITY."
#endif

#ifndef LIBRERTOS_SOFTWARETIMERS
#define LIBRERTOS_SOFTWARETIMERS 0 /* boolean */
#endif

#ifndef LIBRERTOS_STATE_GUARDS
#define LIBRERTOS_STATE_GUARDS 0 /* boolean */
#endif

#ifndef LIBRERTOS_STATISTICS
#define LIBRERTOS_STATISTICS 0 /* boolean */
#endif

#ifndef LIBRERTOS_TEST_CONCURRENT_ACCESS
#define LIBRERTOS_TEST_CONCURRENT_ACCESS()
#endif

typedef void *task_parameter_t;
typedef void (*task_function_t)(task_parameter_t);

struct task_t;
struct task_list_node_t;

struct task_head_list_t {
  struct task_list_node_t *head_ptr;
  struct task_list_node_t *tail_ptr;
  uint8_t length;
};

struct task_list_node_t {
  struct task_list_node_t *next_ptr;
  struct task_list_node_t *prev_ptr;
  tick_t value;
  struct task_head_list_t *list_ptr;
  void *owner_ptr;
};

enum task_state_t {
  TASKSTATE_UNINITIALIZED = 0,
  TASKSTATE_READY,
  TASKSTATE_BLOCKED,
  TASKSTATE_SUSPENDED
};

struct task_t {
  enum task_state_t state;
  task_function_t function;
  task_parameter_t parameter;
  priority_t priority;
  struct task_list_node_t node_delay;
  struct task_list_node_t node_event;

#if (LIBRERTOS_STATISTICS != 0)
  stattime_t task_run_time;
  stattime_t task_num_sched;
#endif
};

#if (LIBRERTOS_SOFTWARETIMERS != 0)

struct timer_t;

typedef void *timer_parameter_t;
typedef void (*timer_function_t)(struct timer_t *, timer_parameter_t);

enum timer_type_t {
  TIMERTYPE_ONESHOT =
      0, /* Timer need to be reset to run. Run after period has passed. */
  TIMERTYPE_AUTO, /* Auto reset timer after it has run. */
  TIMERTYPE_NOPERIOD /* Timer need to be reset to run. Run as soon as it is reset. */
};

struct timer_t {
  enum timer_type_t type;
  tick_t period;
  timer_function_t function;
  timer_parameter_t parameter;
  struct task_list_node_t node_timer;
};

#endif

struct event_r_t {
  struct task_head_list_t list_read;
};

struct event_rw_t {
  struct task_head_list_t list_read;
  struct task_head_list_t list_write;
};

struct semaphore_t {
  len_t count;
  len_t max;
  struct event_r_t event;
};

void LibrertosInit(void);
void LibrertosStart(void);
void LibrertosScheduler(void);
void LibrertosTick(void);

void LibrertosTaskCreate(struct task_t *ptr, priority_t priority,
                task_function_t function, task_parameter_t parameter);
#if (LIBRERTOS_SOFTWARETIMERS != 0)
void LibrertosTimerTaskCreate(priority_t priority);
#endif

tick_t GetTickCount(void);
void TaskDelay(tick_t ticks_to_delay);
void TaskResume(struct task_t *ptr);
struct task_t *GetCurrentTask(void);

void SchedulerLock(void);
void SchedulerUnlock(void);

#if (LIBRERTOS_SOFTWARETIMERS != 0)
void TimerInit(struct timer_t *ptr, enum timer_type_t type, tick_t period,
               timer_function_t function, timer_parameter_t parameter);
void TimerStart(struct timer_t *ptr);
void TimerReset(struct timer_t *ptr);
void TimerStop(struct timer_t *ptr);
bool_t TimerIsRunning(const struct timer_t *ptr);
#endif

#if (LIBRERTOS_STATE_GUARDS != 0)
bool_t LibrertosStateCheck(void);
#endif

#if (LIBRERTOS_STATISTICS != 0)
extern stattime_t LibrertosSystemRunTime(void);
stattime_t LibrertosTotalRunTime(void);
stattime_t LibrertosNoTaskRunTime(void);
stattime_t LibrertosTaskRunTime(const struct task_t *ptr);
stattime_t LibrertosTaskNumSchedules(const struct task_t *ptr);
#endif

void SemaphoreInit(struct semaphore_t *ptr, len_t count, len_t max);
bool_t SemaphoreGive(struct semaphore_t *ptr);
bool_t SemaphoreTake(struct semaphore_t *ptr);
bool_t SemaphoreTakePend(struct semaphore_t *ptr, tick_t ticks_to_wait);
void SemaphorePend(struct semaphore_t *ptr, tick_t ticks_to_wait);
len_t SemaphoreGetCount(const struct semaphore_t *ptr);
len_t SemaphoreGetMax(const struct semaphore_t *ptr);

struct mutex_t {
  len_t count;
  struct task_t *mutex_owner_ptr;
  struct event_r_t event;
};

void MutexInit(struct mutex_t *ptr);
bool_t MutexUnlock(struct mutex_t *ptr);
bool_t MutexLock(struct mutex_t *ptr);
bool_t MutexLockPend(struct mutex_t *ptr, tick_t ticks_to_wait);
void MutexPend(struct mutex_t *ptr, tick_t ticks_to_wait);
len_t MutexGetCount(const struct mutex_t *ptr);
struct task_t *MutexGetOwner(const struct mutex_t *ptr);

struct queue_t {
  len_t item_size;
  len_t free;
  len_t used;
  len_t w_lock;
  len_t r_lock;
  uint8_t *head_ptr;
  uint8_t *tail_ptr;
  uint8_t *buff_ptr;
  uint8_t *buff_end_ptr;
  struct event_rw_t event;
};

void QueueInit(struct queue_t *ptr, void *buff_ptr, len_t length,
               len_t item_size);
bool_t QueueRead(struct queue_t *ptr, void *buff_ptr);
bool_t QueueReadPend(struct queue_t *ptr, void *buff_ptr, tick_t ticks_to_wait);
void QueuePendRead(struct queue_t *ptr, tick_t ticks_to_wait);
bool_t QueueWrite(struct queue_t *ptr, const void *buff_ptr);
bool_t QueueWritePend(struct queue_t *ptr, const void *buff_ptr,
                      tick_t ticks_to_wait);
void QueuePendWrite(struct queue_t *ptr, tick_t ticks_to_wait);
len_t QueueUsed(const struct queue_t *ptr);
len_t QueueFree(const struct queue_t *ptr);
len_t QueueLength(const struct queue_t *ptr);
len_t QueueItemSize(const struct queue_t *ptr);
bool_t QueueEmpty(const struct queue_t *ptr);
bool_t QueueFull(const struct queue_t *ptr);

struct fifo_t {
  len_t length;
  len_t free;
  len_t used;
  len_t w_lock;
  len_t r_lock;
  uint8_t *head_ptr;
  uint8_t *tail_ptr;
  uint8_t *buff_ptr;
  uint8_t *buff_end_ptr;
  struct event_rw_t event;
};

void FifoInit(struct fifo_t *ptr, void *buff_ptr, len_t length);
bool_t FifoReadByte(struct fifo_t *ptr, void *buff_ptr);
len_t FifoRead(struct fifo_t *ptr, void *buff_ptr, len_t length);
len_t FifoReadPend(struct fifo_t *ptr, void *buff_ptr, len_t length,
                   tick_t ticks_to_wait);
void FifoPendRead(struct fifo_t *ptr, len_t length, tick_t ticks_to_wait);
bool_t FifoWriteByte(struct fifo_t *ptr, const void *buff_ptr);
len_t FifoWrite(struct fifo_t *ptr, const void *buff_ptr, len_t length);
len_t FifoWritePend(struct fifo_t *ptr, const void *buff_ptr, len_t length,
                    tick_t ticks_to_wait);
void FifoPendWrite(struct fifo_t *ptr, len_t length, tick_t ticks_to_wait);
len_t FifoUsed(const struct fifo_t *ptr);
len_t FifoFree(const struct fifo_t *ptr);
len_t FifoLength(const struct fifo_t *ptr);
bool_t FifoEmpty(const struct fifo_t *ptr);
bool_t FifoFull(const struct fifo_t *ptr);

#define LIBRERTOS_NO_TASK_RUNNING -1

#ifdef __cplusplus
}
#endif

#endif /* LIBRERTOS_H_ */
