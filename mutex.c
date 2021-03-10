/*
 LibreRTOS - Portable single-stack Real Time Operating System.

 Mutex. Recursive mutex. No priority inheritance mechanism.

 Copyright 2016-2021 Djones A. Boni

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

#define MUTEX_NOT_OWNED ((struct task_t*)NULL)

/** Initialize mutex.

 Initialize mutex:
 Mutex_init(&mtx)
 */
void Mutex_init(struct Mutex_t* o)
{
    o->Count = 0;
    o->MutexOwner = MUTEX_NOT_OWNED;
    OS_eventRInit(&o->Event);
}

/** Lock mutex.

 Can be called only by tasks.

 The task that owns the mutex can lock it multiple times; to completely
 unlock the mutex the task must unlock it the same number of times it was
 locked.

 @return 1 if success, 0 otherwise.

 Lock a mutex:
 Mutex_lock(&mtx)
 */
bool_t Mutex_lock(struct Mutex_t* o)
{
    bool_t val;

    INTERRUPTS_DISABLE();
    {
        struct task_t* currentTask = OS_getCurrentTask();
        val = o->Count == 0 || o->MutexOwner == currentTask;
        if(val != 0)
        {
            ++o->Count;
            o->MutexOwner = currentTask;
        }
    }
    INTERRUPTS_ENABLE();

    return val;
}

/** Unlock mutex.

 The task that owns the mutex can lock it multiple times; to completely
 unlock the mutex the task must unlock it the same number of times it was
 locked.

 @return 1 if success, 0 otherwise.

 Unlock a mutex:
 Mutex_unlock(&mtx)
 */
bool_t Mutex_unlock(struct Mutex_t* o)
{
    bool_t val;
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        val = o->Count > 0;

        if(val != 0)
        {
            --o->Count;

            OS_schedulerLock();

            if(o->Count == 0)
            {
                o->MutexOwner = NULL;

                if(o->Event.ListRead.Length != 0)
                {
                    /* Unblock tasks waiting to read from this event. */
                    OS_eventUnblockTasks(&(o->Event.ListRead));
                }
            }
        }
    }
    CRITICAL_EXIT();

    if(val != 0)
        OS_schedulerUnlock();

    return val;
}

/** Lock or pend on mutex.

 Try lock mutex; pend on it not successful.

 Can be called only by tasks.

 The task will not run until the mutex is unlocked or the timeout expires.

 @param ticksToWait Number of ticks the task will wait for the mutex
 (timeout). Passing MAX_DELAY the task will not wakeup by timeout.
 @return 1 if success, 0 otherwise.

 Lock or pend on mutex without timeout:
 Mutex_lockPend(&mtx, MAX_DELAY)

 Lock or pend on mutex with timeout of 10 ticks:
 Mutex_lockPend(&mtx, 10)
 */
bool_t Mutex_lockPend(struct Mutex_t* o, tick_t ticksToWait)
{
    bool_t val = Mutex_lock(o);
    if(val == 0)
    {
        Mutex_pend(o, ticksToWait);
    }
    return val;
}

/** Pend on mutex.

 Can be called only by tasks.

 The task will not run until the mutex is unlocked or the timeout expires.

 @param ticksToWait Number of ticks the task will wait for the mutex
 (timeout). Passing MAX_DELAY the task will not wakeup by timeout.

 Lock or pend on mutex without timeout:
 Mutex_pend(&mtx, MAX_DELAY)

 Lock or pend on mutex with timeout of 10 ticks:
 Mutex_pend(&mtx, 10)
 */
void Mutex_pend(struct Mutex_t* o, tick_t ticksToWait)
{
    if(ticksToWait != 0U)
    {
        struct task_t* task = OS_getCurrentTask();

        OS_schedulerLock();
        INTERRUPTS_DISABLE();
        if(o->Count != 0 && o->MutexOwner != task)
        {
            OS_eventPrePendTask(&o->Event.ListRead, task);
            INTERRUPTS_ENABLE();
            OS_eventPendTask(&o->Event.ListRead, task, ticksToWait);
        }
        else
        {
            INTERRUPTS_ENABLE();
        }
        OS_schedulerUnlock();
    }
}

/** Get mutex count value.

 @return Number of times the owner of the mutex recursively locked it.

 Get mutex count value:
 Mutex_getCount(&mtx)
 */
len_t Mutex_getCount(const struct Mutex_t* o)
{
    len_t val;
    CRITICAL_VAL();
    CRITICAL_ENTER();
    {
        val = o->Count;
    }
    CRITICAL_EXIT();
    return val;
}

/** Get mutex owner.

 @return Pointer to the task that locked the mutex.

 Get mutex owner:
 Mutex_getOwner(&mtx)
 */
struct task_t* Mutex_getOwner(const struct Mutex_t* o)
{
    struct task_t* val;
    CRITICAL_VAL();
    CRITICAL_ENTER();
    {
        val = o->MutexOwner;
    }
    CRITICAL_EXIT();
    return val;
}
