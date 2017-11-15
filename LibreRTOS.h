/*
 LibreRTOS - Portable single-stack Real Time Operating System.

 Copyright 2016 Djones A. Boni

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

#include <stdint.h>
#include "projdefs.h"

#ifndef LIBRERTOS_MAX_PRIORITY
#define LIBRERTOS_MAX_PRIORITY       3  /* integer > 0 */
#endif

#ifndef LIBRERTOS_PREEMPTION
#define LIBRERTOS_PREEMPTION         0  /* boolean */
#endif

#ifndef LIBRERTOS_PREEMPT_LIMIT
#define LIBRERTOS_PREEMPT_LIMIT      0  /* integer >= 0, < LIBRERTOS_MAX_PRIORITY */
#endif

#if (LIBRERTOS_PREEMPT_LIMIT < 0)
#error "LIBRERTOS_PREEMPT_LIMIT < 0! Should be >= 0."
#endif

#if (LIBRERTOS_PREEMPT_LIMIT >= LIBRERTOS_MAX_PRIORITY)
#error "LIBRERTOS_PREEMPT_LIMIT >= LIBRERTOS_MAX_PRIORITY! Makes the kernel cooperative! Should be < LIBRERTOS_MAX_PRIORITY."
#endif

#ifndef LIBRERTOS_SOFTWARETIMERS
#define LIBRERTOS_SOFTWARETIMERS     0  /* boolean */
#endif

#ifndef LIBRERTOS_STATE_GUARDS
#define LIBRERTOS_STATE_GUARDS       0  /* boolean */
#endif

#ifndef LIBRERTOS_STATISTICS
#define LIBRERTOS_STATISTICS         0  /* boolean */
#endif

#ifndef LIBRERTOS_TEST_CONCURRENT_ACCESS
#define LIBRERTOS_TEST_CONCURRENT_ACCESS()
#endif

typedef void* taskParameter_t;
typedef void(*taskFunction_t)(taskParameter_t);

struct task_t;
struct taskListNode_t;

struct taskHeadList_t {
    struct taskListNode_t* Head;
    struct taskListNode_t* Tail;
    uint8_t                Length;
};

struct taskListNode_t {
    struct taskListNode_t* Next;
    struct taskListNode_t* Previous;
    tick_t                 Value;
    struct taskHeadList_t* List;
    void*                  Owner;
};

enum taskState_t {
    TASKSTATE_READY = 0,
    TASKSTATE_BLOCKED,
    TASKSTATE_SUSPENDED,
    TASKSTATE_NOTINITIALIZED
};

struct task_t {
    enum taskState_t      State;
    taskFunction_t        Function;
    taskParameter_t       Parameter;
    priority_t            Priority;
    struct taskListNode_t NodeDelay;
    struct taskListNode_t NodeEvent;

    #if (LIBRERTOS_STATISTICS != 0)
        stattime_t        TaskRunTime;
        stattime_t        TaskNumSchedules;
    #endif
};



#if (LIBRERTOS_SOFTWARETIMERS != 0)

struct Timer_t;

typedef void* timerParameter_t;
typedef void(*timerFunction_t)(struct Timer_t*, timerParameter_t);

enum timerType_t {
    TIMERTYPE_ONESHOT  = 0x00, /* Timer need to be reset to run. Run after period has passed. */
    TIMERTYPE_AUTO     = 0x01, /* Auto reset timer after it has run. */
    TIMERTYPE_NOPERIOD = 0x02 /* Timer need to be reset to run. Run as soon as it is reset. */
};

struct Timer_t {
    enum timerType_t      Type;
    tick_t                Period;
    timerFunction_t       Function;
    timerParameter_t      Parameter;
    struct taskListNode_t NodeTimer;
};

#endif

struct libreRtosState_t {
    #if (LIBRERTOS_STATE_GUARDS != 0)
        uint32_t               Guard0;
    #endif

    volatile schedulerLock_t   SchedulerLock; /* Scheduler lock. Controls if another task can be scheduled. */
    volatile schedulerLock_t   SchedulerUnlockTodo; /* Flags the schedule unlock function has work to do. */
    struct task_t*volatile     CurrentTCB; /* Current task control block. */

    #if (LIBRERTOS_PREEMPTION != 0)
        volatile bool_t        HigherReadyTask; /* Higher priority task is ready to run. */
    #endif

    struct task_t*             Task[LIBRERTOS_MAX_PRIORITY]; /* Task priorities. */

    tick_t                     Tick; /* OS tick. */
    tick_t                     DelayedTicks; /* OS delayed tick (scheduler was locked). */
    struct taskHeadList_t*     BlockedTaskList_NotOverflowed; /* List with blocked tasks (not overflowed). */
    struct taskHeadList_t*     BlockedTaskList_Overflowed; /* List with blocked tasks (overflowed). */

    struct taskHeadList_t      PendingReadyTaskList; /* List with ready tasks not removed from list of blocked tasks. */
    struct taskHeadList_t      BlockedTaskList1; /* List with blocked tasks number 1. */
    struct taskHeadList_t      BlockedTaskList2; /* List with blocked tasks number 2. */

    #if (LIBRERTOS_SOFTWARETIMERS != 0)
        tick_t                 TaskTimerLastRun;
        struct task_t          TaskTimerTCB; /* Task control block of timer task. */
        struct taskListNode_t* TimerIndex; /* Points to next timer to be run in TimerList. */
        struct taskHeadList_t  TimerList; /* List of running timers ordered by wakeup time. */
        struct taskHeadList_t  TimerUnorderedList; /* List of running timers to be ordered by wakeup time. */
    #endif

    #if (LIBRERTOS_STATISTICS != 0)
        stattime_t        TotalRunTime;
        stattime_t        NoTaskRunTime;
    #endif

    #if (LIBRERTOS_STATE_GUARDS != 0)
        uint32_t               GuardEnd;
    #endif
};

extern struct libreRtosState_t OSstate;



void OS_init(void);
void OS_start(void);
void OS_tick(void);
void OS_scheduler(void);

void OS_schedulerLock(void);
void OS_schedulerUnlock(void);

void OS_taskCreate(
        struct task_t* task,
        priority_t priority,
        taskFunction_t function,
        taskParameter_t parameter);

void OS_taskDelay(tick_t ticksToDelay);
void OS_taskResume(struct task_t* task);

struct task_t* OS_getCurrentTask(void);
tick_t OS_getTickCount(void);

#if (LIBRERTOS_SOFTWARETIMERS != 0)

void OS_timerTaskCreate(priority_t priority);

void Timer_init(
        struct Timer_t* timer,
        enum timerType_t type,
        tick_t period,
        timerFunction_t function,
        timerParameter_t parameter);

void Timer_start(struct Timer_t* timer);
void Timer_reset(struct Timer_t* timer);
void Timer_stop(struct Timer_t* timer);

bool_t Timer_isRunning(const struct Timer_t* timer);

#endif

#if (LIBRERTOS_STATE_GUARDS != 0)

bool_t OS_stateCheck(void);

#endif

#if (LIBRERTOS_STATISTICS != 0)

extern stattime_t US_systemRunTime(void);

stattime_t OS_totalRunTime(void);
stattime_t OS_noTaskRunTime(void);
stattime_t OS_taskRunTime(const struct task_t* task);
stattime_t OS_taskNumSchedules(const struct task_t* task);

#endif



struct eventR_t {
    struct taskHeadList_t ListRead;
};

struct eventRw_t {
    struct taskHeadList_t ListRead;
    struct taskHeadList_t ListWrite;
};



struct Semaphore_t {
    len_t           Count;
    len_t           Max;
    struct eventR_t Event;
};

void Semaphore_init(struct Semaphore_t* o, len_t count, len_t max);

bool_t Semaphore_give(struct Semaphore_t* o);
bool_t Semaphore_take(struct Semaphore_t* o);
bool_t Semaphore_takePend(struct Semaphore_t* o, tick_t ticksToWait);
void Semaphore_pend(struct Semaphore_t* o, tick_t ticksToWait);

len_t Semaphore_getCount(const struct Semaphore_t* o);
len_t Semaphore_getMax(const struct Semaphore_t* o);



struct Mutex_t {
    len_t           Count;
    struct task_t*  MutexOwner;
    struct eventR_t Event;
};

void Mutex_init(struct Mutex_t* o);

bool_t Mutex_unlock(struct Mutex_t* o);
bool_t Mutex_lock(struct Mutex_t* o);
bool_t Mutex_lockPend(struct Mutex_t* o, tick_t ticksToWait);
void Mutex_pend(struct Mutex_t* o, tick_t ticksToWait);

len_t Mutex_getCount(const struct Mutex_t* o);
struct task_t* Mutex_getOwner(const struct Mutex_t* o);



struct Queue_t {
    len_t             ItemSize;
    len_t             Free;
    len_t             Used;
    len_t             WLock;
    len_t             RLock;
    uint8_t*          Head;
    uint8_t*          Tail;
    uint8_t*          Buff;
    uint8_t*          BufEnd;
    struct eventRw_t  Event;
};

void Queue_init(struct Queue_t *o, void *buff, len_t length, len_t item_size);

bool_t Queue_read(struct Queue_t* o, void* buff);
bool_t Queue_readPend(struct Queue_t* o, void* buff, tick_t ticksToWait);
void Queue_pendRead(struct Queue_t* o, tick_t ticksToWait);

bool_t Queue_write(struct Queue_t* o, const void* buff);
bool_t Queue_writePend(struct Queue_t* o, const void* buff, tick_t ticksToWait);
void Queue_pendWrite(struct Queue_t* o, tick_t ticksToWait);

len_t Queue_used(const struct Queue_t *o);
len_t Queue_free(const struct Queue_t *o);
len_t Queue_length(const struct Queue_t *o);
len_t Queue_itemSize(const struct Queue_t *o);

#define Queue_empty(o) (Queue_used(o) == 0)
#define Queue_full(o)  (Queue_free(o) == 0)



struct Fifo_t {
    len_t             Length;
    len_t             Free;
    len_t             Used;
    len_t             WLock;
    len_t             RLock;
    uint8_t*          Head;
    uint8_t*          Tail;
    uint8_t*          Buff;
    uint8_t*          BufEnd;
    struct eventRw_t  Event;
};

void Fifo_init(struct Fifo_t *o, void *buff, len_t length);

bool_t Fifo_readByte(struct Fifo_t* o, void* buff);
len_t Fifo_read(struct Fifo_t* o, void* buff, len_t length);
len_t Fifo_readPend(struct Fifo_t* o, void* buff, len_t length, tick_t ticksToWait);
void Fifo_pendRead(struct Fifo_t* o, len_t length, tick_t ticksToWait);

bool_t Fifo_writeByte(struct Fifo_t* o, const void* buff);
len_t Fifo_write(struct Fifo_t* o, const void* buff, len_t length);
len_t Fifo_writePend(struct Fifo_t* o, const void* buff, len_t length, tick_t ticksToWait);
void Fifo_pendWrite(struct Fifo_t* o, len_t length, tick_t ticksToWait);

len_t Fifo_used(const struct Fifo_t *o);
len_t Fifo_free(const struct Fifo_t *o);
len_t Fifo_length(const struct Fifo_t *o);

#define Fifo_empty(o) (Fifo_used(o) == 0)
#define Fifo_full(o)  (Fifo_free(o) == 0)



#define LIBRERTOS_NO_TASK_RUNNING  -1

#ifdef __cplusplus
}
#endif

#endif /* LIBRERTOS_H_ */
