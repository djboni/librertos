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

enum taskState_t {
    TASKSTATE_READY = 0,
    TASKSTATE_BLOCKED,
    TASKSTATE_NOTINITIALIZED
};

struct task_t {
    enum taskState_t TaskState;
    taskFunction_t   TaskFunction;
    taskParameter_t  TaskParameter;
};

#ifndef LIBRERTOS_NUMBER_OF_TASKS
#define LIBRERTOS_NUMBER_OF_TASKS 4
#endif

static struct librertos_state_t {
    int8_t        SchedulerLock; /* Scheduler lock. Controls if another task can be scheduled. */
    int8_t        CurrentTaskPriority; /* Current task priority. */
    struct task_t Task[LIBRERTOS_NUMBER_OF_TASKS]; /* Task data. */
} state;

/** Initialize OS. Must be called before any other OS function. */
void OS_init(void)
{
    int8_t i;

    state.SchedulerLock = 0;
    state.CurrentTaskPriority = -1;

    for(i = 0; i < LIBRERTOS_NUMBER_OF_TASKS; ++i)
    {
        state.Task[i].TaskState = TASKSTATE_NOTINITIALIZED;
        state.Task[i].TaskFunction = (taskFunction_t)0;
        state.Task[i].TaskParameter = (taskParameter_t)0;
    }
}

/** Schedule a task. May be called in the main loop and after interrupt
 handling. */
void OS_schedule(void)
{
    INTERRUPTS_DISABLE();

    if(OS_schedulerIsLocked() != 0)
    {
        /* Scheduler locked. Cannot run other task. */
        INTERRUPTS_ENABLE();
    }
    else
    {
        int8_t priority;

        do {
            /* Schedule higher priority task. */

            for(    priority = LIBRERTOS_NUMBER_OF_TASKS - 1;
                    priority > state.CurrentTaskPriority;
                    --priority)
            {
                if(state.Task[priority].TaskState == TASKSTATE_READY)
                    break;
            }
            if(priority > state.CurrentTaskPriority)
            {
                /* Higher priority task ready. */

                taskFunction_t taskFunction;
                taskParameter_t taskParameter;

                /* Save last task priority. */
                int8_t lastTaskPrio = state.CurrentTaskPriority;

                /* Set current task priority. */
                state.CurrentTaskPriority = priority;

                /* Get current task function and parameter. */
                taskFunction = state.Task[priority].TaskFunction;
                taskParameter = state.Task[priority].TaskParameter;

                INTERRUPTS_ENABLE();

                /* Run task. */
                taskFunction(taskParameter);

                INTERRUPTS_DISABLE();
                /* Restore last task priority. */
                state.CurrentTaskPriority = lastTaskPrio;
            }
            else
            {
                /* No higher priority task ready. */
                INTERRUPTS_ENABLE();
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

/** Unlock scheduler (recursive lock). Current task may be preempted if
 scheduler is unlocked. */
void OS_schedulerUnlock(void)
{
    --state.SchedulerLock;

    #if (LIBRERTOS_PREEMPTION != 0)
        OS_schedule();
    #endif
}

/** Create task. */
void OS_taskCreate(
        int8_t priority,
        taskFunction_t function,
        taskParameter_t parameter)
{
    ASSERT(priority < LIBRERTOS_NUMBER_OF_TASKS);
    ASSERT(state.Task[priority].TaskState == TASKSTATE_NOTINITIALIZED);

    state.Task[priority].TaskFunction = function;
    state.Task[priority].TaskParameter = parameter;
    state.Task[priority].TaskState = TASKSTATE_READY;
}

/** Delete task. */
void OS_taskDelete(int8_t priority)
{
    ASSERT(priority < LIBRERTOS_NUMBER_OF_TASKS);
    ASSERT(state.Task[priority].TaskState != TASKSTATE_NOTINITIALIZED);

    state.Task[priority].TaskState = TASKSTATE_NOTINITIALIZED;
    state.Task[priority].TaskFunction = (taskFunction_t)0;
    state.Task[priority].TaskParameter = (taskParameter_t)0;
}

/** Return current task priority. */
int8_t OS_taskGetCurrentPriority(void)
{
    return state.CurrentTaskPriority;
}
