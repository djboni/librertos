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

#ifndef LIBRERTOS_NUM_TASKS
#define LIBRERTOS_NUM_TASKS          2  /* integer > 0 */
#endif

#ifndef LIBRERTOS_PREEMPTION
#define LIBRERTOS_PREEMPTION         0  /* boolean */
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

    uint8_t                    TaskCounter; /* Counts number of tasks created. */
    struct task_t              TaskControlBlocks[LIBRERTOS_NUM_TASKS]; /* Task data. */
};

extern struct libreRtosState_t OSstate;



void OS_init(void);
void OS_start(void);
void OS_tick(void);
void OS_scheduler(void);

void OS_schedulerLock(void);
void OS_schedulerUnlock(void);

struct task_t* OS_taskCreate(
        priority_t priority,
        taskFunction_t function,
        taskParameter_t parameter);

void OS_taskDelay(tick_t ticksToDelay);

struct task_t* OS_getCurrentTask(void);
tick_t OS_getTickCount(void);



struct eventR_t {
    struct taskHeadList_t ListRead;
};

struct eventRw_t {
    struct taskHeadList_t ListRead;
    struct taskHeadList_t ListWrite;
};

#define LIBRERTOS_NO_TASK_RUNNING  -1

#ifdef __cplusplus
}
#endif

#endif /* LIBRERTOS_H_ */
