/*
 LibreRTOS - Portable single-stack Real Time Operating System.

 Semaphore. Binary and counter semaphores.

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
#include "OSevent.h"

/** Initialize semaphore.

 @param count Initial count value.
 @param max Max count value.

 Taken binary semaphore:
 Semaphore_init(&sem, 0, 1)

 Counting semaphore with maximum count 3:
 Semaphore_init(&sem, 3, 3)
 */
void Semaphore_init(struct Semaphore_t* o, uint8_t count, uint8_t max)
{
    o->Count = count;
    o->Max = max;
    OS_eventRInit(&o->Event);
}

/** Take semaphore.

 Decrement the semaphore count. Succeed only if the semaphore has been given
 (count is greater than zero).

 @return 1 if success, 0 otherwise.

 Take a semaphore:
 Semaphore_take(&sem)
 */
uint8_t Semaphore_take(struct Semaphore_t* o)
{
    uint8_t val;
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        val = (o->Count > 0);
        if(val != 0)
        {
            o->Count = (uint8_t)(o->Count - 1);
        }
    }
    CRITICAL_EXIT();

    return val;
}

/** Give semaphore.

 Increment the semaphore count. Succeed only if the semaphore has not reached
 its maximum count value.

 @return 1 if success, 0 otherwise.

 Give a semaphore:
 Semaphore_give(&sem)
 */
uint8_t Semaphore_give(struct Semaphore_t* o)
{
    uint8_t val;

    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        val = (o->Count < o->Max);
        if(val != 0)
        {
            o->Count = (uint8_t)(o->Count + 1);

            OS_schedulerLock();

            if(o->Event.ListRead.Length != 0)
            {
                /* Unblock tasks waiting to read from this event. */
                OS_eventUnblockTasks(&(o->Event.ListRead));
            }
        }
    }
    CRITICAL_EXIT();

    if(val != 0)
        OS_schedulerUnlock();

    return val;
}

/** Take or pend on semaphore.

 Try take semaphore; pend on it not successful.

 Can be called only by tasks.

 The task will not run until the semaphore is given or the timeout expires.

 @param ticksToWait Number of ticks the task will wait for the semaphore
 (timeout). Passing MAX_DELAY the task will not wakeup by timeout.
 @return 1 if success, 0 otherwise.

 Take or pend on semaphore without timeout:
 Semaphore_takePend(&sem, MAX_DELAY)

 Take or pend on semaphore with timeout of 10 ticks:
 Semaphore_takePend(&sem, 10)
 */
uint8_t Semaphore_takePend(struct Semaphore_t* o, tick_t ticksToWait)
{
    uint8_t val = Semaphore_take(o);
    if(val == 0)
        Semaphore_pend(o, ticksToWait);
    return val;
}

/** Pend on semaphore.

 Can be called only by tasks.

 The task will not run until the semaphore is given or the timeout expires.

 @param ticksToWait Number of ticks the task will wait for the semaphore
 (timeout). Passing MAX_DELAY the task will not wakeup by timeout.

 Pend on semaphore without timeout:
 Semaphore_pend(&sem, MAX_DELAY)

 Pend on semaphore with timeout of 10 ticks:
 Semaphore_pend(&sem, 10)
 */
void Semaphore_pend(struct Semaphore_t* o, tick_t ticksToWait)
{
    if(ticksToWait != 0U)
    {
        struct task_t* task = OS_getCurrentTask();
        CRITICAL_VAL();

        OS_schedulerLock();
        CRITICAL_ENTER();
        if(o->Count == 0U)
        {
            OS_eventPrePendTask(&o->Event.ListRead, task);
            CRITICAL_EXIT();
            OS_eventPendTask(&o->Event.ListRead, task, ticksToWait);
        }
        else
        {
            CRITICAL_EXIT();
        }
        OS_schedulerUnlock();
    }
}

/** Get semaphore count value.

 @return Semaphore count value.

 Get semaphore count value:
 Semaphore_getCount(&sem)
 */
uint8_t Semaphore_getCount(const struct Semaphore_t* o)
{
    uint8_t val;
    CRITICAL_VAL();
    CRITICAL_ENTER();
    {
        val = o->Count;
    }
    CRITICAL_EXIT();
    return val;
}

/** Get semaphore maximum count value.

 @return Semaphore maximum count value.

 Get semaphore maximum count value:
 Semaphore_getMax(&sem)
 */
uint8_t Semaphore_getMax(const struct Semaphore_t* o)
{
    /* This value is constant after initialization. No need for locks. */
    return o->Max;
}
