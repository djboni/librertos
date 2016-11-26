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

static struct librertos_state_t {
    volatile schedulerLock_t   SchedulerLock; /* Scheduler lock. Controls if another task can be scheduled. */
    struct task_t*             CurrentTaskControlBlock; /* Current task control block. */

    struct taskHeadList_t      PendingReadyTaskList; /* List with ready tasks not removed from list of blocked tasks. */

    #if (LIBRERTOS_TICK != 0)
        tick_t                 Tick; /* OS tick. */
        tick_t                 DelayedTicks; /* OS delayed tick (scheduler was locked). */
        struct taskHeadList_t* BlockedTaskList_NotOverflowed; /* List with blocked tasks (not overflowed). */
        struct taskHeadList_t* BlockedTaskList_Overflowed; /* List with blocked tasks (overflowed). */
        struct taskHeadList_t  BlockedTaskList1; /* List with blocked tasks number 1. */
        struct taskHeadList_t  BlockedTaskList2; /* List with blocked tasks number 2. */
    #endif

    struct task_t*             Task[LIBRERTOS_MAX_PRIORITY]; /* Task priorities. */
    struct task_t              TaskControlBlocks[LIBRERTOS_NUM_TASKS]; /* Task data. */
} state;

#if (LIBRERTOS_TICK != 0)
    static void _OS_tickInvertBlockedTasksLists(void);
    static void _OS_tickUnblockTimedoutTasks(void);
#endif

static void _OS_schedulerUnlock_withoutPreempt(void);

/** Initialize OS. Must be called before any other OS function. */
void OS_init(void)
{
    priority_t priority;

    state.SchedulerLock = 1;
    state.CurrentTaskControlBlock = NULL;

    OS_listHeadInit(&state.PendingReadyTaskList);

    #if (LIBRERTOS_TICK != 0)
        state.Tick = 0U;
        state.DelayedTicks = 0U;
        state.BlockedTaskList_NotOverflowed = &state.BlockedTaskList1;
        state.BlockedTaskList_Overflowed = &state.BlockedTaskList2;
        OS_listHeadInit(&state.BlockedTaskList1);
        OS_listHeadInit(&state.BlockedTaskList2);
    #endif

	for(priority = 0; priority < LIBRERTOS_MAX_PRIORITY; ++priority)
	{
		state.Task[priority] = NULL;
	}

    for(priority = 0; priority < LIBRERTOS_NUM_TASKS; ++priority)
    {
    	state.TaskControlBlocks[priority].TaskState = TASKSTATE_NOTINITIALIZED;
    	state.TaskControlBlocks[priority].TaskFunction = (taskFunction_t)NULL;
    	state.TaskControlBlocks[priority].TaskParameter = (taskParameter_t)NULL;
    	state.TaskControlBlocks[priority].TaskPriority = 0;

        #if (LIBRERTOS_TICK != 0)
            OS_listNodeInit(&state.TaskControlBlocks[priority].TaskBlockedNode, &state.TaskControlBlocks[priority]);
        #endif

        OS_listNodeInit(&state.TaskControlBlocks[priority].TaskEventNode , &state.TaskControlBlocks[priority]);
    }
}

/** Start OS. Must be called once before the scheduler. */
void OS_start(void)
{
    _OS_schedulerUnlock_withoutPreempt();
}

#if (LIBRERTOS_TICK != 0)

    /* Invert blocked tasks lists. Called by unblock timedout tasks function. */
    static void _OS_tickInvertBlockedTasksLists(void)
    {
        struct taskHeadList_t* temp = state.BlockedTaskList_NotOverflowed;
        state.BlockedTaskList_NotOverflowed = state.BlockedTaskList_Overflowed;
        state.BlockedTaskList_Overflowed = temp;
    }

    /* Unblock tasks that have timedout (process OS ticks). Called by scheduler
     unlock function. */
    static void _OS_tickUnblockTimedoutTasks(void)
    {
        /* Unblock tasks that have timed-out. */

        if(state.Tick == 0)
            _OS_tickInvertBlockedTasksLists();

        while(  state.BlockedTaskList_NotOverflowed->ListLength != 0 &&
                state.BlockedTaskList_NotOverflowed->ListHead->TickToWakeup == state.Tick)
        {
            struct taskListNode_t* node = state.BlockedTaskList_NotOverflowed->ListHead;
            OS_listRemove(node);
            state.Task[node->TaskControlBlock->TaskPriority]->TaskState = TASKSTATE_READY;
        }
    }

    /** Increment OS tick. Called by the tick interrupt (defined by the
     user). */
    void OS_tick(void)
    {
        OS_schedulerLock();
        ++state.DelayedTicks;
        OS_schedulerUnlock();
    }

#endif /* LIBRERTOS_TICK */

/** Schedule a task. May be called in the main loop and after interrupt
 handling. */
void OS_scheduler(void)
{
    /* Atomically test and lock scheduler. */
    INTERRUPTS_DISABLE();

    if(OS_schedulerIsLocked() != 0)
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

            struct task_t* thisTask;
            uint8_t higherPriorityTaskReady = 0U;
            priority_t currentTaskPriority = OS_getCurrentPriority();
            priority_t priority;

            for(    priority = LIBRERTOS_MAX_PRIORITY - 1;
                    priority > currentTaskPriority;
                    --priority)
            {
                /* Atomically test TaskState. */
                INTERRUPTS_DISABLE();
                if(		state.Task[priority] != NULL &&
                		state.Task[priority]->TaskState == TASKSTATE_READY)
                {
                    /* Higher priority task ready. */
                    thisTask = state.Task[priority];
                    INTERRUPTS_ENABLE();
                    higherPriorityTaskReady = 1U;
                    break;
                }
                INTERRUPTS_ENABLE();
            }
            if(higherPriorityTaskReady)
            {
                /* Higher priority task ready. */

                /* Get task function and parameter. */
                taskFunction_t taskFunction = thisTask->TaskFunction;
                taskParameter_t taskParameter = thisTask->TaskParameter;

                /* Save last task priority and set current task priority. */
                struct task_t* lastTask = OS_getCurrentTask();
                INTERRUPTS_DISABLE();
                state.CurrentTaskControlBlock = thisTask;
                INTERRUPTS_ENABLE();

                /* Run task. */
                _OS_schedulerUnlock_withoutPreempt();
                taskFunction(taskParameter);
                OS_schedulerLock();

                /* Restore last task priority. */
                INTERRUPTS_DISABLE();
                state.CurrentTaskControlBlock = lastTask;
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

/** Check if scheduler is locked. Return 0 if it is NOT locked, 1 otherwise. */
uint8_t OS_schedulerIsLocked(void)
{
    return state.SchedulerLock != 0;
}

/** Lock scheduler (recursive lock). Current task cannot be preempted if
 scheduler is locked. */
void OS_schedulerLock(void)
{
    ++state.SchedulerLock;
}

/* Unlock scheduler (recursive lock). */
static void _OS_schedulerUnlock_withoutPreempt(void)
{
    CRITICAL_VAL();

    CRITICAL_ENTER();
    while(state.PendingReadyTaskList.ListLength != 0)
    {
        struct taskListNode_t* node = state.PendingReadyTaskList.ListHead;
        OS_listRemove(node);
        CRITICAL_EXIT();
        state.Task[node->TaskControlBlock->TaskPriority]->TaskState = TASKSTATE_READY;
        CRITICAL_ENTER();
    }
    CRITICAL_EXIT();

    #if (LIBRERTOS_TICK != 0)
        if(state.SchedulerLock == 1)
        {
            while(state.DelayedTicks != 0)
            {
                CRITICAL_VAL();
                CRITICAL_ENTER();
                {
                    --state.DelayedTicks;
                    ++state.Tick;
                }
                CRITICAL_EXIT();

                _OS_tickUnblockTimedoutTasks();
            }
        }
    #endif

    --state.SchedulerLock;
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
        priority_t priority,
        taskFunction_t function,
        taskParameter_t parameter)
{
	static uint8_t taskCounter = 0;
	uint8_t task;
	CRITICAL_VAL();

	CRITICAL_ENTER();
	{
		task = taskCounter++;
	}
	CRITICAL_EXIT();

    ASSERT(priority < LIBRERTOS_MAX_PRIORITY);
    ASSERT(state.Task[priority] == NULL);

    ASSERT(task < LIBRERTOS_NUM_TASKS);
    ASSERT(state.TaskControlBlocks[task].TaskState == TASKSTATE_NOTINITIALIZED);

    state.TaskControlBlocks[task].TaskFunction = function;
    state.TaskControlBlocks[task].TaskParameter = parameter;
    state.TaskControlBlocks[task].TaskPriority = priority;

    #if (LIBRERTOS_TICK != 0)
        OS_listNodeInit(&state.TaskControlBlocks[task].TaskBlockedNode, &state.TaskControlBlocks[task]);
    #endif
    OS_listNodeInit(&state.TaskControlBlocks[task].TaskEventNode , &state.TaskControlBlocks[task]);

    state.TaskControlBlocks[task].TaskState = TASKSTATE_READY;

    state.Task[priority] = &state.TaskControlBlocks[task];

    #if (LIBRERTOS_PREEMPTION != 0)
    {
        priority_t currentTaskPriority = OS_getCurrentPriority();
        if(     priority > currentTaskPriority &&
                currentTaskPriority != LIBRERTOS_NO_TASK_RUNNING)
            OS_scheduler();
    }
    #endif
}

/** Return current task priority. */
struct task_t* OS_getCurrentTask(void)
{
	struct task_t* task;
	CRITICAL_VAL();
	CRITICAL_ENTER();
    {
		task = state.CurrentTaskControlBlock;
    }
    CRITICAL_EXIT();
    return task;
}

/** Return current task priority. */
priority_t OS_getCurrentPriority(void)
{
	priority_t priority;
	CRITICAL_VAL();
	CRITICAL_ENTER();
    {
        if(state.CurrentTaskControlBlock != NULL)
		    priority = state.CurrentTaskControlBlock->TaskPriority;
        else
            priority = LIBRERTOS_NO_TASK_RUNNING;
    }
    CRITICAL_EXIT();
    return priority;
}

void OS_taskSuspend(priority_t priority)
{
    ASSERT(priority < LIBRERTOS_MAX_PRIORITY);
    ASSERT(state.Task[priority]->TaskState != TASKSTATE_NOTINITIALIZED);

    state.Task[priority]->TaskState = TASKSTATE_SUSPENDED;
}

void OS_taskResume(priority_t priority)
{
    ASSERT(priority < LIBRERTOS_MAX_PRIORITY);
    ASSERT(state.Task[priority]->TaskState != TASKSTATE_NOTINITIALIZED);

    state.Task[priority]->TaskState = TASKSTATE_READY;

    #if (LIBRERTOS_PREEMPTION != 0)
    {
        if(priority > OS_getCurrentPriority())
            OS_scheduler();
    }
    #endif
}

#if (LIBRERTOS_TICK != 0)

    /** Delay task. */
    void OS_taskDelay(tick_t ticksToDelay)
    {
        /* Insert task in the blocked tasks list. */

        OS_schedulerLock();
        if(ticksToDelay != 0)
        {
            tick_t tickNow = (tick_t)(state.Tick + state.DelayedTicks);
            tick_t tickToWakeup = (tick_t)(tickNow + ticksToDelay);

            struct taskHeadList_t* blockedTaskList;
            struct taskListNode_t* taskBlockedNode =
                    &state.Task[OS_getCurrentPriority()]->TaskBlockedNode;

            if(tickToWakeup > tickNow)
            {
                /* Not overflowed. */
                blockedTaskList = state.BlockedTaskList_NotOverflowed;
            }
            else
            {
                /* Overflowed. */
                blockedTaskList = state.BlockedTaskList_Overflowed;
            }

            /* Insert task on list. */
            OS_listInsert(blockedTaskList, taskBlockedNode, tickToWakeup);

            state.Task[OS_getCurrentPriority()]->TaskState = TASKSTATE_BLOCKED;
        }
        OS_schedulerUnlock();
    }

    /* Get current OS tick. */
    tick_t OS_getTickCount(void)
    {
        tick_t tickNow;
        OS_schedulerLock();
        tickNow = state.Tick;
        OS_schedulerUnlock();
        return tickNow;
    }

#endif /* LIBRERTOS_TICK */



#define LIST_HEAD(x) ((struct taskListNode_t*)x)

void OS_listHeadInit(struct taskHeadList_t* list)
{
    /* Use the list head as a node. */
    list->ListHead = LIST_HEAD(list);
    list->ListTail = LIST_HEAD(list);
    list->TickToWakeup_Zero = 0U;
    list->ListLength = 0;
}

void OS_listNodeInit(struct taskListNode_t* node, struct task_t* taskControlBlock)
{
    node->ListNext = NULL;
    node->ListPrevious = NULL;
    node->TickToWakeup = 0;
    node->ListInserted = NULL;
    node->TaskControlBlock = taskControlBlock;
}

void OS_listInsert(
        struct taskHeadList_t* list,
        struct taskListNode_t* node,
        tick_t tickToWakeup)
{
    struct taskListNode_t* pos = list->ListHead;

    while(pos != LIST_HEAD(list))
    {
        if(tickToWakeup >= pos->TickToWakeup)
        {
            /* Not here. */
            pos = pos->ListNext;
        }
        else
        {
            /* Insert here. */
            break;
        }
    }

    node->TickToWakeup = tickToWakeup;
    node->ListInserted = list;

    node->ListNext = pos;
    node->ListPrevious = pos->ListPrevious;

    pos->ListPrevious->ListNext = node;
    pos->ListPrevious = node;

    ++list->ListLength;
}

void OS_listInsertAfter(
        struct taskHeadList_t* list,
        struct taskListNode_t* pos,
        struct taskListNode_t* node)
{
    node->ListInserted = list;

    node->ListNext = pos->ListNext;
    node->ListPrevious = pos;

    pos->ListNext->ListPrevious = node;
    pos->ListNext = node;

    ++list->ListLength;
}

void OS_listRemove(struct taskListNode_t* node)
{
    struct taskListNode_t* next = node->ListNext;
    struct taskListNode_t* previous = node->ListPrevious;

    next->ListPrevious = previous;
    previous->ListNext = next;

    node->ListNext = NULL;
    node->ListPrevious = NULL;

    --node->ListInserted->ListLength;
    node->ListInserted = NULL;
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

    struct taskListNode_t* node = &task->TaskEventNode;
    OS_listInsertAfter(list, list->ListHead, node);
}

/* Pend task on an event (part 2). Must be called with interrupts enabled and
 scheduler locked. Parameter ticksToWait must not be zero. */
void OS_eventPendTask(
        struct taskHeadList_t* list,
        struct task_t* task,
        tick_t ticksToWait)
{
    struct taskListNode_t* node = &task->TaskEventNode;
    priority_t priority = task->TaskPriority;
    CRITICAL_VAL();

    CRITICAL_ENTER();

    /* Find correct position for the task in the event list. This list may
     be changed by interrupts, so we must do things carefully. */
    {
        struct taskListNode_t* pos;

        for(;;)
        {
            pos = node->ListNext;

            while(pos != LIST_HEAD(list))
            {
                CRITICAL_EXIT();
                if(pos->TaskControlBlock->TaskPriority >= priority)
                {
                    /* Found where to insert. Break while(). */
                    CRITICAL_ENTER();
                    break;
                }

                CRITICAL_ENTER();
                if(pos->ListInserted != list)
                {
                    /* This position was removed from the list. An interrupt
                     resumed this task. Break while(). */
                    break;
                }

                /* As this task is inserted in the tail of the list, if an
                 interrupt resumed the task then pos also must have been
                 modified.
                 So break the while loop if current task was changed is
                 redundant. */

                pos = pos->ListNext;
            }

            if(     pos != LIST_HEAD(list) &&
                    pos->ListInserted != list &&
                    node->ListInserted == list)
            {
                /* This pos was removed from the list and pxEvent was not
                 removed. Must restart to find where to insert node.
                 Continue for(;;). */
                continue;
            }
            else
            {
                /* Found where to insert. Insert before pos.
                 OR
                 Item node was removed from the list (interrupt resumed the
                 task). Nothing to insert.
                 Break for(;;). */
                break;
            }
        }

        if(node->ListInserted == list)
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
        }
    }
    CRITICAL_EXIT();

    /* Suspend or block task. */
    #if (LIBRERTOS_TICK == 0)
    {
        /* Ticks disabled. Suspend. */
		state.Task[OS_getCurrentPriority()]->TaskState = TASKSTATE_SUSPENDED;
    }
    #else
    {
        /* Ticks enabled. Suspend if ticks to wait is maximum delay, block with
         timeout otherwise. */
        if(ticksToWait == MAX_DELAY)
            state.Task[OS_getCurrentPriority()]->TaskState = TASKSTATE_SUSPENDED;
        else
            OS_taskDelay(ticksToWait);
    }
    #endif
}

/* Unblock task in an event list. Must be called with scheduler locked and in a
 critical section. */
void OS_eventUnblockTasks(struct taskHeadList_t* list)
{
    if(list->ListLength != 0)
    {
        struct taskListNode_t* node = list->ListTail;

        /* Remove from event list. */
        OS_listRemove(node);

        /* Insert in the pending ready tasks . */
        OS_listInsertAfter(&state.PendingReadyTaskList, state.PendingReadyTaskList.ListHead, node);
    }
}

struct taskListNode_t* OS_getTaskEventNode(priority_t priority)
{
	return &state.Task[priority]->TaskEventNode;
}
