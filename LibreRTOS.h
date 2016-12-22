/*
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

#ifndef LIBRERTOS_STATE_GUARDS
#define LIBRERTOS_STATE_GUARDS       0  /* boolean */
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
    struct task_t*         Task;
};

enum taskState_t {
    TASKSTATE_READY = 0,
    TASKSTATE_RUNNING,
    TASKSTATE_BLOCKED,
    TASKSTATE_SUSPENDED,
    TASKSTATE_NOTINITIALIZED
};

struct task_t {
    enum taskState_t          State;
    taskFunction_t            Function;
    taskParameter_t           Parameter;
    priority_t                Priority;
    struct taskListNode_t     NodeDelay;
    struct taskListNode_t     NodeEvent;
};

struct libreRtosState_t {
    #if (LIBRERTOS_STATE_GUARDS != 0)
        uint32_t               Guard0;
    #endif

    volatile schedulerLock_t   SchedulerLock; /* Scheduler lock. Controls if another task can be scheduled. */
    struct task_t*             CurrentTCB; /* Current task control block. */
    struct task_t*             Task[LIBRERTOS_MAX_PRIORITY]; /* Task priorities. */

    tick_t                     Tick; /* OS tick. */
    tick_t                     DelayedTicks; /* OS delayed tick (scheduler was locked). */
    struct taskHeadList_t*     BlockedTaskList_NotOverflowed; /* List with blocked tasks (not overflowed). */
    struct taskHeadList_t*     BlockedTaskList_Overflowed; /* List with blocked tasks (overflowed). */

    struct taskHeadList_t      PendingReadyTaskList; /* List with ready tasks not removed from list of blocked tasks. */
    struct taskHeadList_t      BlockedTaskList1; /* List with blocked tasks number 1. */
    struct taskHeadList_t      BlockedTaskList2; /* List with blocked tasks number 2. */

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


#if (LIBRERTOS_STATE_GUARDS != 0)

    uint8_t OS_stateCheck(void);

#endif



struct eventR_t {
    struct taskHeadList_t ListRead;
};

struct eventRw_t {
    struct taskHeadList_t ListRead;
    struct taskHeadList_t ListWrite;
};



struct Semaphore_t {
    volatile uint8_t Count;
    uint8_t          Max;
    struct eventR_t  Event;
};

void Semaphore_init(struct Semaphore_t* o, uint8_t count, uint8_t max);

uint8_t Semaphore_give(struct Semaphore_t* o);
uint8_t Semaphore_take(struct Semaphore_t* o);
uint8_t Semaphore_takePend(struct Semaphore_t* o, tick_t ticksToWait);
void Semaphore_pend(struct Semaphore_t* o, tick_t ticksToWait);

uint8_t Semaphore_getCount(struct Semaphore_t* o);
uint8_t Semaphore_getMax(struct Semaphore_t* o);



struct Mutex_t {
    volatile uint8_t Count;
    struct task_t*   MutexOwner;
    struct eventR_t  Event;
};

void Mutex_init(struct Mutex_t* o);

uint8_t Mutex_unlock(struct Mutex_t* o);
uint8_t Mutex_lock(struct Mutex_t* o);
uint8_t Mutex_lockPend(struct Mutex_t* o, tick_t ticksToWait);
void Mutex_pend(struct Mutex_t* o, tick_t ticksToWait);



struct Queue_t {
    uint8_t              ItemSize;
    volatile uint8_t     Free;
    volatile uint8_t     Used;
    volatile uint8_t     WLock;
    volatile uint8_t     RLock;
    uint8_t *volatile    Head;
    uint8_t *volatile    Tail;
    uint8_t*             Buff;
    uint8_t*             BufEnd;
    struct eventRw_t     Event;
};

void Queue_init(
        struct Queue_t *o,
        void *buff,
        uint8_t length,
        uint8_t item_size);

uint8_t Queue_read(struct Queue_t* o, void* buff);
uint8_t Queue_readPend(struct Queue_t* o, void* buff, tick_t ticksToWait);
void Queue_pendRead(struct Queue_t* o, tick_t ticksToWait);

uint8_t Queue_write(struct Queue_t* o, const void* buff);
uint8_t Queue_writePend(struct Queue_t* o, const void* buff, tick_t ticksToWait);
void Queue_pendWrite(struct Queue_t* o, tick_t ticksToWait);

uint8_t Queue_used(const struct Queue_t *o);
uint8_t Queue_free(const struct Queue_t *o);

#define Queue_empty(o) (Queue_used(o) == 0)
#define Queue_full(o)  (Queue_free(o) == 0)



struct Fifo_t {
    uint8_t              Length;
    volatile uint8_t     Free;
    volatile uint8_t     Used;
    volatile uint8_t     WLock;
    volatile uint8_t     RLock;
    uint8_t *volatile    Head;
    uint8_t *volatile    Tail;
    uint8_t*             Buff;
    uint8_t*             BufEnd;
    struct eventRw_t     Event;
};

void Fifo_init(
        struct Fifo_t *o,
        void *buff,
        uint8_t length);

uint8_t Fifo_read(struct Fifo_t* o, void* buff, uint8_t length);
uint8_t Fifo_readPend(struct Fifo_t* o, void* buff, uint8_t length, tick_t ticksToWait);
void Fifo_pendRead(struct Fifo_t* o, uint8_t length, tick_t ticksToWait);

uint8_t Fifo_write(struct Fifo_t* o, const void* buff, uint8_t length);
uint8_t Fifo_writePend(struct Fifo_t* o, const void* buff, uint8_t length, tick_t ticksToWait);
void Fifo_pendWrite(struct Fifo_t* o, uint8_t length, tick_t ticksToWait);

uint8_t Fifo_used(const struct Fifo_t *o);
uint8_t Fifo_free(const struct Fifo_t *o);

#define Fifo_empty(o) (Fifo_used(o) == 0)
#define Fifo_full(o)  (Fifo_free(o) == 0)



#define LIBRERTOS_NO_TASK_RUNNING  -1

#ifdef __cplusplus
}
#endif

#endif /* LIBRERTOS_H_ */
