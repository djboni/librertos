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
#include <stddef.h>

enum taskState_t {
    TASKSTATE_READY = 0,
    TASKSTATE_BLOCKED,
    TASKSTATE_SUUSPENDED,
    TASKSTATE_NOTINITIALIZED
};

struct task_t {
    enum taskState_t          TaskState;
    taskFunction_t            TaskFunction;
    taskParameter_t           TaskParameter;

    #if (LIBRERTOS_TICK != 0)
        struct taskListNode_t TaskBlockedNode;
    #endif
};

static struct librertos_state_t {
    schedulerLock_t       SchedulerLock; /* Scheduler lock. Controls if another task can be scheduled. */
    priority_t            CurrentTaskPriority; /* Current task priority. */

    #if (LIBRERTOS_TICK != 0)
        tick_t            Tick; /* OS tick. */
        tick_t            DelayedTicks; /* OS delayed tick (scheduler was locked). */
        struct taskHeadList_t BlockedTaskList1; /* List with blocked tasks. */
        struct taskHeadList_t BlockedTaskList2; /* List with blocked tasks (overflowed). */
    #endif

    struct task_t         Task[LIBRERTOS_MAX_PRIORITY]; /* Task data. */
} state;

static void _OS_tickInvertBlockedTasksLists(void);
static void _OS_tickUnblockTimedoutTasks(void);
static void _OS_schedulerUnlock_withoutPreempt(void);

/** Initialize OS. Must be called before any other OS function. */
void OS_init(void)
{
    priority_t priority;

    state.SchedulerLock = 0;
    state.CurrentTaskPriority = LIBRERTOS_SCHEDULER_NOT_RUNNING;

    #if (LIBRERTOS_TICK != 0)
        state.Tick = 0U;
        state.DelayedTicks = 0U;
        OS_listHeadInit(&state.BlockedTaskList1);
        OS_listHeadInit(&state.BlockedTaskList2);
    #endif

    for(priority = 0; priority < LIBRERTOS_MAX_PRIORITY; ++priority)
    {
        state.Task[priority].TaskState = TASKSTATE_NOTINITIALIZED;
        state.Task[priority].TaskFunction = (taskFunction_t)NULL;
        state.Task[priority].TaskParameter = (taskParameter_t)NULL;

        #if (LIBRERTOS_TICK != 0)
            OS_listNodeInit(&state.Task[priority].TaskBlockedNode, priority);
        #endif
    }
}

#if (LIBRERTOS_TICK != 0)

    /* Invert blocked tasks lists. Called by unblock timedout tasks function. */
    static void _OS_tickInvertBlockedTasksLists(void)
    {
        struct taskHeadList_t temp = state.BlockedTaskList1;
        state.BlockedTaskList2 = state.BlockedTaskList1;
        state.BlockedTaskList1 = temp;
    }

    /* Unblock tasks that have timedout (process OS ticks). Called by scheduler
     unlock function. */
    static void _OS_tickUnblockTimedoutTasks(void)
    {
        /* Unblock tasks that have timed-out. */

        if(state.Tick == 0)
            _OS_tickInvertBlockedTasksLists();

        while(  state.BlockedTaskList1.ListLength != 0 &&
                state.BlockedTaskList1.ListHead->TickToWakeup == state.Tick)
        {
            struct taskListNode_t* node = state.BlockedTaskList1.ListHead;
            OS_listRemove(node);
            state.Task[node->TaskPriority].TaskState = TASKSTATE_READY;
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

            priority_t priority;

            for(    priority = LIBRERTOS_MAX_PRIORITY - 1;
                    priority > state.CurrentTaskPriority;
                    --priority)
            {
                /* CurrentTaskPriority cannot change when scheduler is locked.
                 No need to disable interrupts.
                 Task[].TaskState may change even with scheduler locked. Need to
                 disable interrupts only if TaskState load is not atomic. To be
                 on the safe side, interrupts are ALWAYS disabled to load
                 TaskState. */

                /* Atomically test TaskState. */
                INTERRUPTS_DISABLE();
                if(state.Task[priority].TaskState == TASKSTATE_READY)
                {
                    INTERRUPTS_ENABLE();
                    break;
                }
                INTERRUPTS_ENABLE();
            }
            if(priority > state.CurrentTaskPriority)
            {
                /* Higher priority task ready. */

                /* Save last task priority and set current task priority. */
                priority_t lastTaskPrio = state.CurrentTaskPriority;
                state.CurrentTaskPriority = priority;

                #if (LIBRERTOS_ENABLE_TASKDELETE == 0)
                {
                    /* Get current task function and parameter. */
                    taskFunction_t taskFunction = state.Task[priority].TaskFunction;
                    taskParameter_t taskParameter = state.Task[priority].TaskParameter;

                    _OS_schedulerUnlock_withoutPreempt();

                    /* Run task. */
                    taskFunction(taskParameter);

                    OS_schedulerLock();
                }
                #else /* LIBRERTOS_ENABLE_TASKDELETE */
                {
                    /* Selected task may have been deleted. */

                    /* Atomically test if task was deleted and read function and
                     parameter. */
                    INTERRUPTS_DISABLE();
                    if(state.Task[priority].TaskState == TASKSTATE_READY)
                    {
                        /* Task was not deleted. */

                        /* Get current task function and parameter. */
                        taskFunction_t taskFunction = state.Task[priority].TaskFunction;
                        taskParameter_t taskParameter = state.Task[priority].TaskParameter;

                        INTERRUPTS_ENABLE();
                        _OS_schedulerUnlock_withoutPreempt();

                        /* Run task. */
                        taskFunction(taskParameter);

                        OS_schedulerLock();
                    }
                    else
                    {
                        /* Task was deleted. */
                        INTERRUPTS_ENABLE();
                    }
                }
                #endif /* LIBRERTOS_ENABLE_TASKDELETE */

                /* Restore last task priority. */
                state.CurrentTaskPriority = lastTaskPrio;
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
int8_t OS_schedulerIsLocked(void)
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
    ASSERT(priority < LIBRERTOS_MAX_PRIORITY);
    ASSERT(state.Task[priority].TaskState == TASKSTATE_NOTINITIALIZED);

    state.Task[priority].TaskFunction = function;
    state.Task[priority].TaskParameter = parameter;
    #if (LIBRERTOS_TICK != 0)
        OS_listNodeInit(&state.Task[priority].TaskBlockedNode, priority);
    #endif

    state.Task[priority].TaskState = TASKSTATE_READY;

    #if (LIBRERTOS_PREEMPTION != 0)
    {
        priority_t currentTaskPriority = OS_taskGetCurrentPriority();
        if(     priority > currentTaskPriority &&
                currentTaskPriority != LIBRERTOS_SCHEDULER_NOT_RUNNING)
            OS_scheduler();
    }
    #endif
}

#if (LIBRERTOS_ENABLE_TASKDELETE != 0)

    /** Delete task. */
    void OS_taskDelete(priority_t priority)
    {
        ASSERT(priority < LIBRERTOS_MAX_PRIORITY);
        ASSERT(state.Task[priority].TaskState != TASKSTATE_NOTINITIALIZED);

        state.Task[priority].TaskState = TASKSTATE_NOTINITIALIZED;
        state.Task[priority].TaskFunction = (taskFunction_t)0;
        state.Task[priority].TaskParameter = (taskParameter_t)0;

        OS_schedulerLock();
        #if (LIBRERTOS_TICK != 0)
            if(state.Task[priority].TaskBlockedNode.ListInserted != NULL)
            {
                /* Remove task from blocked list. */
                OS_listRemove(&state.Task[priority].TaskBlockedNode);
            }
        #endif
        _OS_schedulerUnlock_withoutPreempt();
    }

#endif /* LIBRERTOS_ENABLE_TASKDELETE */

/** Return current task priority. */
priority_t OS_taskGetCurrentPriority(void)
{
    return state.CurrentTaskPriority;
}

void OS_taskSuspend(priority_t priority)
{
    ASSERT(priority < LIBRERTOS_MAX_PRIORITY);
    ASSERT(state.Task[priority].TaskState != TASKSTATE_NOTINITIALIZED);

    state.Task[priority].TaskState = TASKSTATE_SUUSPENDED;
}

void OS_taskResume(priority_t priority)
{
    ASSERT(priority < LIBRERTOS_MAX_PRIORITY);
    ASSERT(state.Task[priority].TaskState != TASKSTATE_NOTINITIALIZED);

    state.Task[priority].TaskState = TASKSTATE_READY;

    #if (LIBRERTOS_PREEMPTION != 0)
    {
        if(priority > OS_taskGetCurrentPriority())
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
                    &state.Task[OS_taskGetCurrentPriority()].TaskBlockedNode;

            if(tickToWakeup > tickNow)
            {
                /* Not overflowed. */
                blockedTaskList = &state.BlockedTaskList1;
            }
            else
            {
                /* Overflowed. */
                blockedTaskList = &state.BlockedTaskList2;
            }

            /* Insert task on list. */
            OS_listInsert(blockedTaskList, taskBlockedNode, tickToWakeup);

            state.Task[OS_taskGetCurrentPriority()].TaskState = TASKSTATE_BLOCKED;
        }
        _OS_schedulerUnlock_withoutPreempt();
    }

#endif /* LIBRERTOS_TICK */
