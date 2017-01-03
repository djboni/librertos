/*
 LibreRTOS - Portable single-stack Real Time Operating System.

 Software timers.

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

#if (LIBRERTOS_SOFTWARETIMERS != 0)

/* Execute a timer. Used by timer task function. */
static void _OS_timerExecute(struct Timer_t* timer)
{
    timerFunction_t function;
    timerParameter_t parameter;

    INTERRUPTS_DISABLE();
    function = timer->Function;
    parameter = timer->Parameter;
    INTERRUPTS_ENABLE();

    function(timer, parameter);
}

/* Insert timer into ordered list. Used by timer task function. */
static void _OS_timerInsertInOrderedList(struct Timer_t* timer, tick_t tickToWakeup)
{
    struct taskListNode_t* pos;
    struct taskListNode_t*const node = &timer->NodeTimer;
    struct taskHeadList_t*const list = &OSstate.TimerList;

    INTERRUPTS_DISABLE();

    for(;;)
    {
        if(tickToWakeup > OSstate.TaskTimerLastRun)
        {
            pos = OSstate.TimerIndex;
        }
        else
        {
            pos = list->Head;
        }

        while(pos != (struct taskListNode_t*)list)
        {
            INTERRUPTS_ENABLE();

            /* For test coverage only. This macro is used as a deterministic
             way to create a concurrent access. */
            LIBRERTOS_TEST_CONCURRENT_ACCESS();

            if(pos->Value >= tickToWakeup)
            {
                /* Found where to insert. Break while(). */
                INTERRUPTS_DISABLE();
                break;
            }

            INTERRUPTS_DISABLE();
            if(pos->List != list)
            {
                /* This position was removed from the list. Break while(). */
                break;
            }

            pos = pos->Next;
        }

        if(     pos != (struct taskListNode_t*)list &&
                pos->List != list &&
                node->List == &OSstate.TimerUnorderedList)
        {
            /* This pos was removed from the list and node was not
             removed. Must restart to find where to insert node.
             Continue for(;;). */
            continue;
        }
        else
        {
            /* Found where to insert. Insert before pos.
             OR
             Item node was removed from the list. Nothing to insert.
             Break for(;;). */
            break;
        }
    }

    if(node->List == &OSstate.TimerUnorderedList)
    {
        /* Timer was not removed from list. */

        /* Now insert in the right position. */
        OS_listRemove(node);
        OS_listInsertAfter(list, pos->Previous, node);
        node->Value = tickToWakeup;

        if(     tickToWakeup > OSstate.TaskTimerLastRun &&
                OSstate.TimerIndex->Previous == &timer->NodeTimer)
        {
            OSstate.TimerIndex = &timer->NodeTimer;
        }
    }

    INTERRUPTS_ENABLE();
}

/* Timer task function. Used by timer task create function. */
static void _OS_timerFunction(taskParameter_t param)
{
    uint8_t changeIndex;
    (void)param;

    {
        tick_t now = OS_getTickCount();
        if(now >= OSstate.TaskTimerLastRun)
        {
            OSstate.TaskTimerLastRun = now;
            changeIndex = 0;
        }
        else
        {
            OSstate.TaskTimerLastRun = MAX_DELAY;
            changeIndex = 1;
        }
    }

    INTERRUPTS_DISABLE();

    /* Insert timers into ordered list; execute one-shot timers. */
    while(OSstate.TimerUnorderedList.Length != 0)
    {
        struct taskListNode_t* node = OSstate.TimerUnorderedList.Head;
        struct Timer_t* timer = (struct Timer_t*)node->Owner;

        if(timer->Type == TIMERTYPE_ONESHOT)
        {
            /* Execute one-shot timer. */
            OS_listRemove(node);
            INTERRUPTS_ENABLE();
            _OS_timerExecute(timer);
        }
        else
        {
            /* Insert timer into ordered list. */
            tick_t tickToWakeup;
            INTERRUPTS_ENABLE();
            tickToWakeup = (tick_t)(OSstate.TaskTimerLastRun + timer->Period);
            _OS_timerInsertInOrderedList(timer, tickToWakeup);
        }
        INTERRUPTS_DISABLE();
    }

    INTERRUPTS_ENABLE();

    /* Execute ready timer. */
    while(  OSstate.TimerIndex != (struct taskListNode_t*)&OSstate.TimerList &&
            OSstate.TimerIndex->Value <= OSstate.TaskTimerLastRun)
    {
        struct taskListNode_t* node = OSstate.TimerIndex;
        struct Timer_t* timer = (struct Timer_t*)node->Owner;
        OSstate.TimerIndex = node->Next;

        if(timer->Type == TIMERTYPE_AUTO)
        {
            Timer_reset(timer);
        }
        else
        {
            INTERRUPTS_DISABLE();
            OS_listRemove(node);
            INTERRUPTS_ENABLE();
        }

        /* Execute timer. */
        _OS_timerExecute(timer);
    }

    INTERRUPTS_DISABLE();

    if(changeIndex != 0)
    {
        /* Change index. Run timer task again. */
        INTERRUPTS_ENABLE();
        OSstate.TimerIndex = OSstate.TimerList.Head;
        OSstate.TaskTimerLastRun = 0;
    }
    else if(OSstate.TimerUnorderedList.Length == 0 &&
            (OSstate.TimerIndex == (struct taskListNode_t*)&OSstate.TimerList ||
            OSstate.TimerIndex->Value > OSstate.Tick))
    {
        /* No timer is ready. Block timer task. */
        tick_t ticksToSleep;
        struct Timer_t* nextTimer;

        OS_schedulerLock();

        if(OSstate.TimerIndex != (struct taskListNode_t*)&OSstate.TimerList)
        {
            nextTimer = OSstate.TimerIndex->Owner;
            ticksToSleep = (tick_t)(nextTimer->NodeTimer.Value - OSstate.Tick);
            INTERRUPTS_ENABLE();
        }
        else if(OSstate.TimerList.Head != (struct taskListNode_t*)&OSstate.TimerList)
        {
            nextTimer = OSstate.TimerList.Head->Owner;
            ticksToSleep = (tick_t)(nextTimer->NodeTimer.Value - OSstate.Tick);
            INTERRUPTS_ENABLE();
        }
        else
        {
            ticksToSleep = MAX_DELAY;
            OS_taskDelay(MAX_DELAY);
        }

        if((difftick_t)ticksToSleep > 0)
        {
            /* Delay task only if timer wakeup time is not in the past.
             If there is not timer the timer task is already delayed. */
            OS_taskDelay(ticksToSleep);
        }

        OS_schedulerUnlock();
    }
    else
    {
        /* A timer is ready. Run timer task again. */
        INTERRUPTS_ENABLE();
    }
}

/** Create timer task.  */
void OS_timerTaskCreate(priority_t priority)
{
    OS_taskCreate(&OSstate.TaskTimerTCB, priority, &_OS_timerFunction, NULL);
}

/** Initialize timer structure.

 This function does not start the timer.

 The types can be of three types: TIMERTYPE_DEFAULT, TIMERTYPE_AUTO,
 TIMERTYPE_ONESHOT.

 A default timer will run once after its period has timed-out after a reset or
 start.

 An auto timer will run multiple times after after a reset or start. The auto
 timer resets itself when it runs.

 An one-shot timer will run once when the timer task is scheduled.

 @param type Define the type of the timer. The types can be TIMERTYPE_DEFAULT,
 TIMERTYPE_AUTO, TIMERTYPE_ONESHOT.
 @param period Period of the timer. After a reset or start the timer will run
 only after its period has timed-out. Note: A one-shot timer does not use the
 period information; pass value 0 for one-shot timer.
 @param function Function pointer to the timer function. It will be called by
 timer task when the timer runs. The prototype must be
 void timerFunction(struct Timer_t* timer, void* param);
 @param parameter Parameter passed to the timer function.
 */
void Timer_init(
        struct Timer_t* timer,
        enum timerType_t type,
        tick_t period,
        timerFunction_t function,
        timerParameter_t parameter)
{
    timer->Type = type;
    timer->Period = period;
    timer->Function = function;
    timer->Parameter = parameter;
    OS_listNodeInit(&timer->NodeTimer, timer);
}

/** Start a timer.

 Reset the timer only if it is not running.
 */
void Timer_start(struct Timer_t* timer)
{
    if(!Timer_isRunning(timer))
    {
        Timer_reset(timer);
    }
}

/** Reset a timer.

 For default and auto timers:
 The timer run only after 'period' ticks have passed after now.

 For one-shot timer:
 The timer run as soon as the timer task is scheduled.
 */
void Timer_reset(struct Timer_t* timer)
{
    CRITICAL_VAL();
    CRITICAL_ENTER();

    if(timer->NodeTimer.List != NULL)
    {
        /* If timer is running. */

        if(OSstate.TimerIndex == &timer->NodeTimer)
        {
            OSstate.TimerIndex = OSstate.TimerIndex->Next;
        }

        OS_listRemove(&timer->NodeTimer);
    }

    OS_listInsertAfter(
            &OSstate.TimerUnorderedList,
            (struct taskListNode_t*)&OSstate.TimerUnorderedList,
            &timer->NodeTimer);

    CRITICAL_EXIT();

    OS_taskResume(&OSstate.TaskTimerTCB);
}

/** Stop a timer.

 The timer will not run again until it is started or reset.
 */
void Timer_stop(struct Timer_t* timer)
{
    CRITICAL_VAL();
    CRITICAL_ENTER();

    if(timer->NodeTimer.List != NULL)
    {
        /* If timer is running. */

        if(OSstate.TimerIndex == &timer->NodeTimer)
        {
            OSstate.TimerIndex = OSstate.TimerIndex->Next;
        }

        OS_listRemove(&timer->NodeTimer);
    }

    CRITICAL_EXIT();
}

/** Check if a timer is running.

 A timer is running if it has been started or reset and has not stopped.

 @return 1 if timer is running, 0 if it is stopped.
 */
bool_t Timer_isRunning(const struct Timer_t* timer)
{
    bool_t x;
    CRITICAL_VAL();
    CRITICAL_ENTER();

    x = timer->NodeTimer.List != NULL;

    CRITICAL_EXIT();
    return x;
}

#endif
