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

#include "LibreRTOS.h"
#include "OSlist.h"
#include "OSevent.h"
#include <stddef.h>

struct libreRtosState_t OSstate;

static void _OS_tickInvertBlockedTasksLists(void);
static void _OS_tickUnblockTimedoutTasks(void);
static void _OS_schedulerUnlock_withoutPreempt(void);

/** Initialize OS. Must be called before any other OS function. */
void OS_init(void)
{
    uint16_t i;

    OSstate.SchedulerLock = 1;
    OSstate.CurrentTCB = NULL;

    OS_listHeadInit(&OSstate.PendingReadyTaskList);

    OSstate.Tick = 0U;
    OSstate.DelayedTicks = 0U;
    OSstate.BlockedTaskList_NotOverflowed = &OSstate.BlockedTaskList1;
    OSstate.BlockedTaskList_Overflowed = &OSstate.BlockedTaskList2;
    OS_listHeadInit(&OSstate.BlockedTaskList1);
    OS_listHeadInit(&OSstate.BlockedTaskList2);

    for(i = 0; i < LIBRERTOS_MAX_PRIORITY; ++i)
    {
        OSstate.Task[i] = NULL;
    }
}

/** Start OS. Must be called once before the scheduler. */
void OS_start(void)
{
    _OS_schedulerUnlock_withoutPreempt();
}

/* Invert blocked tasks lists. Called by unblock timedout tasks function. */
static void _OS_tickInvertBlockedTasksLists(void)
{
    struct taskHeadList_t* temp = OSstate.BlockedTaskList_NotOverflowed;
    OSstate.BlockedTaskList_NotOverflowed = OSstate.BlockedTaskList_Overflowed;
    OSstate.BlockedTaskList_Overflowed = temp;
}

/** Increment OS tick. Called by the tick interrupt (defined by the
 user). */
void OS_tick(void)
{
    OS_schedulerLock();
    ++OSstate.DelayedTicks;
    OS_schedulerUnlock();
}

/** Schedule a task. May be called in the main loop and after interrupt
 handling. */
void OS_scheduler(void)
{
    /* Atomically test and lock scheduler. */
    INTERRUPTS_DISABLE();

    if(OSstate.SchedulerLock != 0)
    {
        /* Scheduler locked. Cannot run other task. */
        INTERRUPTS_ENABLE();
    }
    else
    {
        /* Scheduler unlocked. */

        OS_schedulerLock();
        INTERRUPTS_ENABLE();

        do {
            /* Schedule higher priority task. */

            /* Save current TCB. */
            struct task_t* currentTask = OS_getCurrentTask();
            struct task_t* task = NULL;

            {
                priority_t currentTaskPriority = (currentTask == NULL ?
                        LIBRERTOS_NO_TASK_RUNNING : currentTask->Priority);
                priority_t priority;

                for(    priority = LIBRERTOS_MAX_PRIORITY - 1;
                        priority > currentTaskPriority;
                        --priority)
                {
                    /* Atomically test TaskState. */
                    INTERRUPTS_DISABLE();
                    if(     OSstate.Task[priority] != NULL &&
                            OSstate.Task[priority]->State == TASKSTATE_READY)
                    {
                        /* Higher priority task ready. */
                        task = OSstate.Task[priority];
                        INTERRUPTS_ENABLE();
                        break;
                    }
                    INTERRUPTS_ENABLE();
                }
            }

            if(task != NULL)
            {
                /* Higher priority task ready. */

                /* Get task function and parameter. */
                taskFunction_t taskFunction = task->Function;
                taskParameter_t taskParameter = task->Parameter;

                /* Set current TCB. */
                INTERRUPTS_DISABLE();
                OSstate.CurrentTCB = task;
                INTERRUPTS_ENABLE();

                task->State = TASKSTATE_RUNNING;
                _OS_schedulerUnlock_withoutPreempt();

                /* Run task. */
                taskFunction(taskParameter);

                OS_schedulerLock();
                if(task->State == TASKSTATE_RUNNING)
                    task->State = TASKSTATE_READY;

                /* Restore last TCB. */
                INTERRUPTS_DISABLE();
                OSstate.CurrentTCB = currentTask;
                INTERRUPTS_ENABLE();
            }
            else
            {
                /* No higher priority task ready. */
                _OS_schedulerUnlock_withoutPreempt();
                break;
            }

            /* Loop to check if another higher priority task is ready. */
        } while(1);
    }
}

/** Lock scheduler (recursive lock). Current task cannot be preempted if
 scheduler is locked. */
void OS_schedulerLock(void)
{
    ++OSstate.SchedulerLock;
}

/* Unblock tasks that have timedout (process OS ticks). Called by scheduler
 unlock function. */
static void _OS_tickUnblockTimedoutTasks(void)
{
    /* Unblock tasks that have timed-out. */
    CRITICAL_VAL();

    if(OSstate.Tick == 0)
    {
        _OS_tickInvertBlockedTasksLists();
    }

    while(  OSstate.BlockedTaskList_NotOverflowed->Length != 0 &&
            OSstate.BlockedTaskList_NotOverflowed->Head->Value == OSstate.Tick)
    {
        struct task_t* task = OSstate.BlockedTaskList_NotOverflowed->Head->Task;

        /* Remove from blocked list. */
        OS_listRemove(&task->NodeDelay);

        CRITICAL_ENTER();

        task->State = TASKSTATE_READY;

        /* Remove from event list. */
        if(task->NodeEvent.List != NULL)
        {
            OS_listRemove(&task->NodeEvent);
        }

        CRITICAL_EXIT();
    }
}

/* Unblock pending ready tasks. Called by scheduler unlock function. */
static void _OS_tickUnblockPendingReadyTasks(void)
{
    CRITICAL_VAL();

    CRITICAL_ENTER();
    while(OSstate.PendingReadyTaskList.Length != 0)
    {
        struct task_t* task = OSstate.PendingReadyTaskList.Head->Task;

        /* Remove from pending ready list. */
        OS_listRemove(&task->NodeEvent);

        CRITICAL_EXIT();

        /* Remove from blocked list. */
        if(task->NodeDelay.List != NULL)
        {
            OS_listRemove(&task->NodeDelay);
        }

        task->State = TASKSTATE_READY;

        CRITICAL_ENTER();
    }
    CRITICAL_EXIT();
}

/* Unlock scheduler (recursive lock). */
static void _OS_schedulerUnlock_withoutPreempt(void)
{
    CRITICAL_VAL();

    if(OSstate.SchedulerLock == 1)
    {
        CRITICAL_ENTER();

        while(OSstate.DelayedTicks != 0)
        {
            --OSstate.DelayedTicks;
            ++OSstate.Tick;
            CRITICAL_EXIT();
            _OS_tickUnblockTimedoutTasks();
            CRITICAL_ENTER();
        }

        CRITICAL_EXIT();
    }

    _OS_tickUnblockPendingReadyTasks();

    --OSstate.SchedulerLock;
}

/** Unlock scheduler (recursive lock). Current task may be preempted if
 scheduler is unlocked. */
void OS_schedulerUnlock(void)
{
    _OS_schedulerUnlock_withoutPreempt();

    #if (LIBRERTOS_PREEMPTION != 0)
    {
        OS_scheduler();
    }
    #endif
}

/** Create task. */
void OS_taskCreate(
        struct task_t* task,
        priority_t priority,
        taskFunction_t function,
        taskParameter_t parameter)
{
    ASSERT(priority < LIBRERTOS_MAX_PRIORITY);
    ASSERT(OSstate.Task[priority] == NULL);

    task->State = TASKSTATE_READY;
    task->Function = function;
    task->Parameter = parameter;
    task->Priority = priority;

    OS_listNodeInit(&task->NodeDelay, task);
    OS_listNodeInit(&task->NodeEvent , task);

    OSstate.Task[priority] = task;
}

/** Return current task priority. */
struct task_t* OS_getCurrentTask(void)
{
    struct task_t* task;
    CRITICAL_VAL();
    CRITICAL_ENTER();
    {
        task = OSstate.CurrentTCB;
    }
    CRITICAL_EXIT();
    return task;
}

/** Delay task. */
void OS_taskDelay(tick_t ticksToDelay)
{
    /* Insert task in the blocked tasks list. */
    CRITICAL_VAL();

    OS_schedulerLock();
    if(ticksToDelay != 0)
    {
        tick_t tickNow = (tick_t)(OSstate.Tick + OSstate.DelayedTicks);
        tick_t tickToWakeup = (tick_t)(tickNow + ticksToDelay);

        struct task_t* task = OS_getCurrentTask();
        struct taskListNode_t* taskBlockedNode = &task->NodeDelay;
        struct taskHeadList_t* blockedTaskList;

        if(tickToWakeup > tickNow)
        {
            /* Not overflowed. */
            blockedTaskList = OSstate.BlockedTaskList_NotOverflowed;
        }
        else
        {
            /* Overflowed. */
            blockedTaskList = OSstate.BlockedTaskList_Overflowed;
        }

        CRITICAL_ENTER();
        task->State = TASKSTATE_BLOCKED;
        CRITICAL_EXIT();

        /* Insert task on list. */
        OS_listInsert(blockedTaskList, taskBlockedNode, tickToWakeup);
    }
    OS_schedulerUnlock();
}

/* Get current OS tick. */
tick_t OS_getTickCount(void)
{
    tick_t tickNow;
    OS_schedulerLock();
    tickNow = OSstate.Tick;
    OS_schedulerUnlock();
    return tickNow;
}



#define LIST_HEAD(x) ((struct taskListNode_t*)x)

void OS_listHeadInit(struct taskHeadList_t* list)
{
    /* Use the list head as a node. */
    list->Head = LIST_HEAD(list);
    list->Tail = LIST_HEAD(list);
    list->Length = 0;
}

void OS_listNodeInit(
        struct taskListNode_t* node,
        struct task_t* task)
{
    node->Next = NULL;
    node->Previous = NULL;
    node->Value = 0;
    node->List = NULL;
    node->Task = task;
}

void OS_listInsert(
        struct taskHeadList_t* list,
        struct taskListNode_t* node,
        tick_t value)
{
    struct taskListNode_t* pos = list->Head;

    while(pos != LIST_HEAD(list))
    {
        if(value >= pos->Value)
        {
            /* Not here. */
            pos = pos->Next;
        }
        else
        {
            /* Insert here. */
            break;
        }
    }

    node->Value = value;
    node->List = list;

    node->Next = pos;
    node->Previous = pos->Previous;

    pos->Previous->Next = node;
    pos->Previous = node;

    ++list->Length;
}

void OS_listInsertAfter(
        struct taskHeadList_t* list,
        struct taskListNode_t* pos,
        struct taskListNode_t* node)
{
    node->List = list;

    node->Next = pos->Next;
    node->Previous = pos;

    pos->Next->Previous = node;
    pos->Next = node;

    ++list->Length;
}

void OS_listRemove(struct taskListNode_t* node)
{
    struct taskListNode_t* next = node->Next;
    struct taskListNode_t* previous = node->Previous;

    --node->List->Length;

    next->Previous = previous;
    previous->Next = next;

    node->Next = NULL;
    node->Previous = NULL;
    node->List = NULL;
}



/* Initialize event struct. */
void OS_eventRInit(struct eventR_t* o)
{
    OS_listHeadInit(&o->ListRead);
}

/* Initialize event struct. */
void OS_eventRwInit(struct eventRw_t* o)
{
    OS_listHeadInit(&o->ListRead);
    OS_listHeadInit(&o->ListWrite);
}

/* Pend task on an event (part 1). Must be called with interrupts disabled and
 scheduler locked. */
void OS_eventPrePendTask(
        struct taskHeadList_t* list,
        struct task_t* task)
{
    /* Put task on the head position in the event list. So the task may be
     unblocked by an interrupt. */

    struct taskListNode_t* node = &task->NodeEvent;
    OS_listInsertAfter(list, (struct taskListNode_t*)list, node);
}

/* Pend task on an event (part 2). Must be called with interrupts enabled and
 scheduler locked. Parameter ticksToWait must not be zero. */
void OS_eventPendTask(
        struct taskHeadList_t* list,
        struct task_t* task,
        tick_t ticksToWait)
{
    struct taskListNode_t* node = &task->NodeEvent;
    priority_t priority = task->Priority;
    CRITICAL_VAL();

    CRITICAL_ENTER();

    /* Find correct position for the task in the event list. This list may
     be changed by interrupts, so we must do things carefully. */
    {
        struct taskListNode_t* pos;

        for(;;)
        {
            pos = list->Tail;

            while(pos != LIST_HEAD(list))
            {
                CRITICAL_EXIT();
                if(pos->Task->Priority <= priority)
                {
                    /* Found where to insert. Break while(). */
                    CRITICAL_ENTER();
                    break;
                }

                CRITICAL_ENTER();
                if(pos->List != list)
                {
                    /* This position was removed from the list. An interrupt
                     resumed this task. Break while(). */
                    break;
                }

                /* As this task is inserted in the head of the list, if an
                 interrupt resumed the task then pos also must have been
                 modified.
                 So break the while loop if current task was changed is
                 redundant. */

                pos = pos->Previous;
            }

            if(     pos != LIST_HEAD(list) &&
                    pos->List != list &&
                    node->List == list)
            {
                /* This pos was removed from the list and node was not
                 removed. Must restart to find where to insert node.
                 Continue for(;;). */
                continue;
            }
            else
            {
                /* Found where to insert. Insert after pos.
                 OR
                 Item node was removed from the list (interrupt resumed the
                 task). Nothing to insert.
                 Break for(;;). */
                break;
            }
        }

        if(node->List == list)
        {
            /* If an interrupt didn't resume the task. */

            /* Don't need to remove and insert if the task is already in its
             position. */

            if(node != pos)
            {
                /* Now insert in the right position. */
                OS_listRemove(node);
                OS_listInsertAfter(list, pos, node);
            }

            /* Suspend or block task. */
            /* Ticks enabled. Suspend if ticks to wait is maximum delay, block with
                timeout otherwise. */
            if(ticksToWait == MAX_DELAY)
            {
                task->State = TASKSTATE_SUSPENDED;
                CRITICAL_EXIT();
            }
            else
            {
                CRITICAL_EXIT();
                OS_taskDelay(ticksToWait);
            }
        }
        else
        {
            CRITICAL_EXIT();
        }
    }
}

/* Unblock task in an event list. Must be called with scheduler locked and in a
 critical section. */
void OS_eventUnblockTasks(struct taskHeadList_t* list)
{
    if(list->Length != 0)
    {
        struct taskListNode_t* node = list->Tail;

        /* Remove from event list. */
        OS_listRemove(node);

        /* Insert in the pending ready tasks . */
        OS_listInsertAfter(&OSstate.PendingReadyTaskList, OSstate.PendingReadyTaskList.Head, node);
    }
}
