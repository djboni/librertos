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
#define LIBRERTOS_MAX_PRIORITY       2  /* integer > 0 */
#endif

#ifndef LIBRERTOS_PREEMPTION
#define LIBRERTOS_PREEMPTION         0  /* boolean */
#endif

#ifndef LIBRERTOS_TICK
#define LIBRERTOS_TICK               0  /* boolean */
#endif

#ifndef LIBRERTOS_ENABLE_TASKDELETE
#define LIBRERTOS_ENABLE_TASKDELETE  0  /* boolean */
#endif

#ifndef LIBRERTOS_QUEUE_1CRITICAL
#define LIBRERTOS_QUEUE_1CRITICAL    0  /* boolean */
#endif

#ifndef LIBRERTOS_FIFO_1CRITICAL
#define LIBRERTOS_FIFO_1CRITICAL     0  /* boolean */
#endif

typedef void* taskParameter_t;
typedef void(*taskFunction_t)(taskParameter_t);



void OS_init(void);
void OS_tick(void);
void OS_scheduler(void);

int8_t OS_schedulerIsLocked(void);
void OS_schedulerLock(void);
void OS_schedulerUnlock(void);

void OS_taskCreate(
        priority_t priority,
        taskFunction_t function,
        taskParameter_t parameter);
void OS_taskDelete(priority_t priority);
void OS_taskSuspend(priority_t priority);
void OS_taskResume(priority_t priority);
void OS_taskDelay(tick_t ticksToDelay);

priority_t OS_getCurrentPriority(void);
tick_t OS_getTickCount(void);



struct taskListNode_t;

struct taskHeadList_t {
    struct taskListNode_t* ListHead;
    struct taskListNode_t* ListTail;
    tick_t                 TickToWakeup_Zero;
    int8_t                 ListLength;
};

struct taskListNode_t {
    struct taskListNode_t* ListNext;
    struct taskListNode_t* ListPrevious;
    tick_t                 TickToWakeup;
    priority_t             TaskPriority;
    struct taskHeadList_t* ListInserted;
};



struct eventR_t {
    struct taskHeadList_t ListRead;
};

struct eventRw_t {
    struct taskHeadList_t ListRead;
    struct taskHeadList_t ListWrite;
};



struct Semaphore_t {
    volatile int8_t Count;
    struct eventR_t Event;
};

void Semaphore_init(struct Semaphore_t* o, int8_t count);

void Semaphore_give(struct Semaphore_t* o);
int8_t Semaphore_take(struct Semaphore_t* o);
int8_t Semaphore_takePend(struct Semaphore_t* o, tick_t ticksToWait);

int8_t Semaphore_getCount(struct Semaphore_t* o);



struct Queue_t {
    const int8_t        ItemSize;
    volatile int8_t     Free;
    volatile int8_t     Used;
    #if (LIBRERTOS_QUEUE_1CRITICAL == 0)
        volatile int8_t WLock;
        volatile int8_t RLock;
    #endif
    uint8_t *volatile   Head;
    uint8_t *volatile   Tail;
    uint8_t *const      Buff;
    uint8_t *const      BufEnd;
    struct eventRw_t    Event;
};

void Queue_init(
        struct Queue_t *o,
        void *buff,
        int8_t length,
        int8_t item_size);

int8_t Queue_read(struct Queue_t* o, void* buff);
int8_t Queue_readPend(struct Queue_t* o, void* buff, tick_t ticksToWait);
int8_t Queue_write(struct Queue_t* o, const void* buff);
int8_t Queue_writePend(struct Queue_t* o, const void* buff, tick_t ticksToWait);

int8_t Queue_used(const struct Queue_t *o);
int8_t Queue_free(const struct Queue_t *o);

#define Queue_empty(o) (Queue_used(o) == 0)
#define Queue_full(o)  (Queue_free(o) == 0)



struct Fifo_t {
    const int8_t        Length;
    volatile int8_t     Free;
    volatile int8_t     Used;
    #if (LIBRERTOS_FIFO_1CRITICAL == 0)
        volatile int8_t WLock;
        volatile int8_t RLock;
    #endif
    uint8_t *volatile   Head;
    uint8_t *volatile   Tail;
    uint8_t *const      Buff;
    uint8_t *const      BufEnd;
    struct eventRw_t    Event;
};

void Fifo_init(
        struct Fifo_t *o,
        void *buff,
        int8_t length);

int8_t Fifo_read(struct Fifo_t* o, void* buff, int8_t length);
int8_t Fifo_readPend(struct Fifo_t* o, void* buff, int8_t length, tick_t ticksToWait);
int8_t Fifo_write(struct Fifo_t* o, const void* buff, int8_t length);
int8_t Fifo_writePend(struct Fifo_t* o, const void* buff, int8_t length, tick_t ticksToWait);

int8_t Fifo_used(const struct Fifo_t *o);
int8_t Fifo_free(const struct Fifo_t *o);

#define Fifo_empty(o) (Fifo_used(o) == 0)
#define Fifo_full(o)  (Fifo_free(o) == 0)


#define LIBRERTOS_SCHEDULER_NOT_RUNNING  -1

#ifdef __cplusplus
}
#endif

#endif /* LIBRERTOS_H_ */
