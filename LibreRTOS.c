/*
 LibreRTOS - Portable single-stack Real Time Operating System.

 Scheduler.
 Linked list.
 Pend on events.

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

#ifndef LIBRERTOS_GUARD_U32
#define LIBRERTOS_GUARD_U32 0xFA57C0DEUL
#endif

struct libreRtosState_t OSstate;

static void _OS_tickInvertBlockedTasksLists(void);
static void _OS_tickUnblockTimedoutTasks(void);
static void _OS_tickUnblockPendingReadyTasks(void);
static void _OS_schedulerReal(void);
static void _OS_scheduleTask(struct task_t*const task);

/** Initialize OS. Must be called before any other OS function. */
void OS_init(void)
{
    uint16_t i;

    OSstate.SchedulerLock = 1;
    OSstate.SchedulerUnlockTodo = 0;
    OSstate.CurrentTCB = NULL;

    #if (LIBRERTOS_PREEMPTION != 0)
    {
        OSstate.HigherReadyTask = 0;
    }
    #endif

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

    #if (LIBRERTOS_SOFTWARETIMERS != 0)
    {
        OSstate.TaskTimerLastRun = 0;
        /* OSstate.TaskTimerTCB is initialized when calling OS_timerTaskCreate(). */
        OSstate.TimerIndex = (struct taskListNode_t*)&OSstate.TimerList;
        OS_listHeadInit(&OSstate.TimerList);
        OS_listHeadInit(&OSstate.TimerUnorderedList);
    }
    #endif

    #if (LIBRERTOS_STATISTICS != 0)
    {
        OSstate.TotalRunTime = 0;
        OSstate.NoTaskRunTime = 0;
    }
    #endif

    #if (LIBRERTOS_STATE_GUARDS != 0)
    {
        OSstate.Guard0 = LIBRERTOS_GUARD_U32;
        OSstate.GuardEnd = LIBRERTOS_GUARD_U32;
    }
    #endif
}

/** Start OS. Must be called once before the scheduler. */
void OS_start(void)
{
    INTERRUPTS_ENABLE();
    OS_schedulerUnlock();
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

    /* Scheduler unlock has work todo. */
    OSstate.SchedulerUnlockTodo = 1;

    OS_schedulerUnlock();
}

/** Schedule a task. Called by scheduler. */
static void _OS_scheduleTask(struct task_t*const task)
{
    struct task_t* currentTask;

    /* Get task function and parameter. */
    taskFunction_t taskFunction = task->Function;
    taskParameter_t taskParameter = task->Parameter;

    /* Save and set current TCB. */
    INTERRUPTS_DISABLE();

    /* Inside critical section. We can read CurrentTCB directly. */
    currentTask = OSstate.CurrentTCB;

    OSstate.CurrentTCB = task;

    #if (LIBRERTOS_STATISTICS != 0)
    {
        stattime_t now = US_systemRunTime();
        OSstate.NoTaskRunTime += now - OSstate.TotalRunTime;
        OSstate.TotalRunTime = now;
        ++task->TaskNumSchedules;
    }
    #endif

    INTERRUPTS_ENABLE();

    OS_schedulerUnlock();

    /* Run task. */
    taskFunction(taskParameter);

    OS_schedulerLock();

    /* Restore last TCB. */
    INTERRUPTS_DISABLE();

    OSstate.CurrentTCB = currentTask;

    #if (LIBRERTOS_STATISTICS != 0)
    {
        stattime_t now = US_systemRunTime();
        task->TaskRunTime += now - OSstate.TotalRunTime;
        OSstate.TotalRunTime = now;
    }
    #endif

    INTERRUPTS_ENABLE();
}

/** Schedule a task. Called in the main loop. */
void OS_scheduler(void)
{
    OS_schedulerLock();
    _OS_schedulerReal();
    OS_schedulerUnlock();
}

/** Scheduler algorithm a task. Called by scheduler functions and by scheduler
 unlock function. */
static void _OS_schedulerReal(void)
{
    #if (LIBRERTOS_STATISTICS != 0)
    {
        stattime_t now;
        /* Scheduler locked. We can read CurrentTCB directly. */
        struct task_t* currentTask = OSstate.CurrentTCB;
        INTERRUPTS_DISABLE();
        now = US_systemRunTime();
        if(currentTask != NULL)
        {
            currentTask->TaskRunTime += now - OSstate.TotalRunTime;
        }
        else
        {
            OSstate.NoTaskRunTime += now - OSstate.TotalRunTime;
        }
        OSstate.TotalRunTime = now;
        INTERRUPTS_ENABLE();
    }
    #endif

    for(;;)
    {
        /* Schedule higher priority task. */

        /* Save current TCB. */
        struct task_t* task = NULL;

        {
            /* Scheduler locked. We can read CurrentTCB directly. */
            priority_t currentTaskPriority = (OSstate.CurrentTCB == NULL ?
                    LIBRERTOS_NO_TASK_RUNNING : OSstate.CurrentTCB->Priority);
            priority_t priority;

            for(    priority = LIBRERTOS_MAX_PRIORITY - 1;
                    priority > currentTaskPriority;
                    --priority)
            {
                /* Atomically test TaskState. */
                INTERRUPTS_DISABLE();
                if(OSstate.Task[priority] != NULL)
                {
                    /* Higher priority task ready. */

                    #if (LIBRERTOS_PREEMPTION != 0 && LIBRERTOS_PREEMPT_LIMIT > 0)
                    {
                        /* Schedule only if preemption limit allows it. */
                        if(     priority >= LIBRERTOS_PREEMPT_LIMIT ||
                                currentTaskPriority == LIBRERTOS_NO_TASK_RUNNING)
                        {
                            task = OSstate.Task[priority];
                        }
                    }
                    #else /* LIBRERTOS_PREEMPTION && LIBRERTOS_PREEMPT_LIMIT */
                    {
                        task = OSstate.Task[priority];
                    }
                    #endif /* LIBRERTOS_PREEMPTION && LIBRERTOS_PREEMPT_LIMIT */

                    INTERRUPTS_ENABLE();
                    break;
                }
                INTERRUPTS_ENABLE();
            }
        }

        if(task != NULL)
        {
            /* Higher priority task ready. */
            _OS_scheduleTask(task);
        }
        else
        {
            /* No higher priority task ready. */
            break;
        }

        /* Loop to check if another higher priority task is ready. */

    } /* for(;;) */

    #if (LIBRERTOS_STATISTICS != 0)
    {
        stattime_t now;
        INTERRUPTS_DISABLE();
        now = US_systemRunTime();
        OSstate.NoTaskRunTime += now - OSstate.TotalRunTime;
        OSstate.TotalRunTime = now;
        INTERRUPTS_ENABLE();
    }
    #endif
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

    if(OSstate.Tick == 0)
    {
        _OS_tickInvertBlockedTasksLists();
    }

    while(  OSstate.BlockedTaskList_NotOverflowed->Length != 0 &&
            OSstate.BlockedTaskList_NotOverflowed->Head->Value == OSstate.Tick)
    {
        struct task_t* task = (struct task_t*)OSstate.BlockedTaskList_NotOverflowed->Head->Owner;

        /* Remove from blocked list. */
        OS_listRemove(&task->NodeDelay);

        INTERRUPTS_DISABLE();

        task->State = TASKSTATE_READY;

        /* Remove from event list. */
        if(task->NodeEvent.List != NULL)
        {
            OS_listRemove(&task->NodeEvent);
        }

        OSstate.Task[task->Priority] = task;

        #if (LIBRERTOS_PREEMPTION != 0)
        {
            #if (LIBRERTOS_PREEMPT_LIMIT > 0)
            if(task->Priority >= LIBRERTOS_PREEMPT_LIMIT)
            {
            #endif

                /* Inside critical section. We can read CurrentTCB directly. */
                if(     OSstate.CurrentTCB == NULL ||
                        task->Priority > OSstate.CurrentTCB->Priority)
                {
                    OSstate.HigherReadyTask = 1;
                }

            #if (LIBRERTOS_PREEMPT_LIMIT > 0)
            }
            #endif
        }
        #endif /* LIBRERTOS_PREEMPTION */

        INTERRUPTS_ENABLE();
    }
}

/* Unblock pending ready tasks. Called by scheduler unlock function. */
static void _OS_tickUnblockPendingReadyTasks(void)
{
    INTERRUPTS_DISABLE();
    while(OSstate.PendingReadyTaskList.Length != 0)
    {
        struct task_t* task = (struct task_t*)OSstate.PendingReadyTaskList.Head->Owner;

        /* Remove from pending ready list. */
        OS_listRemove(&task->NodeEvent);

        #if (LIBRERTOS_PREEMPTION != 0)
        {
            #if (LIBRERTOS_PREEMPT_LIMIT > 0)
            if(task->Priority >= LIBRERTOS_PREEMPT_LIMIT)
            {
            #endif

                /* Inside critical section. We can read CurrentTCB directly. */
                if(     OSstate.CurrentTCB == NULL ||
                        task->Priority > OSstate.CurrentTCB->Priority)
                {
                    OSstate.HigherReadyTask = 1;
                }

            #if (LIBRERTOS_PREEMPT_LIMIT > 0)
            }
            #endif
        }
        #endif /* LIBRERTOS_PREEMPTION */

        INTERRUPTS_ENABLE();

        /* Remove from blocked list. */
        if(task->NodeDelay.List != NULL)
        {
            OS_listRemove(&task->NodeDelay);
        }

        task->State = TASKSTATE_READY;

        INTERRUPTS_DISABLE();

        OSstate.Task[task->Priority] = task;
    }
    INTERRUPTS_ENABLE();
}

/** Unlock scheduler (recursive lock). Current task may be preempted if
 scheduler is unlocked. */
void OS_schedulerUnlock(void)
{
    if(OSstate.SchedulerUnlockTodo != 0 && OSstate.SchedulerLock == 1)
    {
        CRITICAL_VAL();
        CRITICAL_ENTER();

        for(;;)
        {
            OSstate.SchedulerUnlockTodo = 0;

            while(OSstate.DelayedTicks != 0)
            {
                --OSstate.DelayedTicks;
                ++OSstate.Tick;

                INTERRUPTS_ENABLE();
                _OS_tickUnblockTimedoutTasks();
                INTERRUPTS_DISABLE();
            }

            INTERRUPTS_ENABLE();

            _OS_tickUnblockPendingReadyTasks();

            INTERRUPTS_DISABLE();

            #if (LIBRERTOS_PREEMPTION != 0)
            {
                if(OSstate.HigherReadyTask != 0)
                {
                    OSstate.HigherReadyTask = 0;
                    INTERRUPTS_ENABLE();
                    _OS_schedulerReal();
                    INTERRUPTS_DISABLE();
                }
            }
            #endif /* LIBRERTOS_PREEMPTION */

            if(OSstate.SchedulerUnlockTodo == 0)
            {
                /* Scheduler unlock has no work todo. */
                break;
            }
        }

        --OSstate.SchedulerLock;

        CRITICAL_EXIT();
    }
    else
    {
        --OSstate.SchedulerLock;
    }
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

    #if (LIBRERTOS_STATISTICS != 0)
    {
        task->TaskRunTime = 0;
        task->TaskNumSchedules = 0;
    }
    #endif

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

        INTERRUPTS_DISABLE();
        task->State = TASKSTATE_BLOCKED;
        OSstate.Task[task->Priority] = NULL;
        INTERRUPTS_ENABLE();

        /* Insert task on list. */
        OS_listInsert(blockedTaskList, taskBlockedNode, tickToWakeup);
    }
    OS_schedulerUnlock();
}

/** Resume task. */
void OS_taskResume(struct task_t* task)
{
    struct taskListNode_t* node = &task->NodeEvent;
    CRITICAL_VAL();

    OS_schedulerLock();
    {
        CRITICAL_ENTER();
        {
            /* Remove from event list. */
            if(node->List != NULL)
            {
                OS_listRemove(&task->NodeEvent);
            }

            /* Add to pending ready tasks list. */
            OS_listInsertAfter(&OSstate.PendingReadyTaskList, OSstate.PendingReadyTaskList.Head, node);

            /* Scheduler unlock has work todo. */
            OSstate.SchedulerUnlockTodo = 1;
        }
        CRITICAL_EXIT();
    }
    OS_schedulerUnlock();
}

/** Get current OS tick. */
tick_t OS_getTickCount(void)
{
    tick_t tickNow;
    OS_schedulerLock();
    tickNow = OSstate.Tick;
    OS_schedulerUnlock();
    return tickNow;
}

#if (LIBRERTOS_STATE_GUARDS != 0)

/** Return 1 if OSstate guards are fine. 0 otherwise. */
bool_t OS_stateCheck(void)
{
    return (OSstate.Guard0 == LIBRERTOS_GUARD_U32 &&
            OSstate.GuardEnd == LIBRERTOS_GUARD_U32);
}

#endif /* LIBRERTOS_STATE_GUARDS */

#if (LIBRERTOS_STATISTICS != 0)

stattime_t OS_totalRunTime(void)
{
    stattime_t val;
    CRITICAL_VAL();
    CRITICAL_ENTER();
    val = OSstate.TotalRunTime;
    CRITICAL_EXIT();
    return val;
}

stattime_t OS_noTaskRunTime(void)
{
    stattime_t val;
    CRITICAL_VAL();
    CRITICAL_ENTER();
    val = OSstate.NoTaskRunTime;
    CRITICAL_EXIT();
    return val;
}

stattime_t OS_taskRunTime(const struct task_t* task)
{
    stattime_t val;
    CRITICAL_VAL();
    CRITICAL_ENTER();
    val = task->TaskRunTime;
    CRITICAL_EXIT();
    return val;
}

stattime_t OS_taskNumSchedules(const struct task_t* task)
{
    stattime_t val;
    CRITICAL_VAL();
    CRITICAL_ENTER();
    val = task->TaskNumSchedules;
    CRITICAL_EXIT();
    return val;
}

#endif /* LIBRERTOS_STATISTICS */



#define LIST_HEAD(x) ((struct taskListNode_t*)x)

/* Initialize list head. */
void OS_listHeadInit(struct taskHeadList_t* list)
{
    /* Use the list head as a node. */
    list->Head = LIST_HEAD(list);
    list->Tail = LIST_HEAD(list);
    list->Length = 0;
}

/* Initialize list node. */
void OS_listNodeInit(
        struct taskListNode_t* node,
        void* owner)
{
    node->Next = NULL;
    node->Previous = NULL;
    node->Value = 0;
    node->List = NULL;
    node->Owner = owner;
}

/* Insert node into list. Position according to value. */
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

/* Insert node into list after pos. */
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

/* Remove node from list. */
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

    INTERRUPTS_DISABLE();

    /* Find correct position for the task in the event list. This list may
     be changed by interrupts, so we must do things carefully. */
    {
        struct taskListNode_t* pos;

        for(;;)
        {
            pos = list->Tail;

            while(pos != LIST_HEAD(list))
            {
                INTERRUPTS_ENABLE();

                /* For test coverage only. This macro is used as a deterministic
                 way to create a concurrent access. */
                LIBRERTOS_TEST_CONCURRENT_ACCESS();

                if(((struct task_t*)pos->Owner)->Priority <= priority)
                {
                    /* Found where to insert. Break while(). */
                    INTERRUPTS_DISABLE();
                    break;
                }

                INTERRUPTS_DISABLE();
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
                OSstate.Task[task->Priority] = NULL;
                INTERRUPTS_ENABLE();
            }
            else
            {
                INTERRUPTS_ENABLE();
                OS_taskDelay(ticksToWait);
            }
        }
        else
        {
            INTERRUPTS_ENABLE();
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

        /* Scheduler unlock has work todo. */
        OSstate.SchedulerUnlockTodo = 1;
    }
}
