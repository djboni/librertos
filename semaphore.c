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
#include "OSevent.h"

#define UNCONST(type, var) *((type*)&(var))

void Semaphore_initbinary(struct Semaphore_t* o)
{
    o->Count = 0;
    UNCONST(int8_t, o->Max) = 1;
    OS_eventInit(&o->Event);
}

void Semaphore_init(struct Semaphore_t* o, int8_t max)
{
    o->Count = 0;
    UNCONST(int8_t, o->Max) = max;
    OS_eventInit(&o->Event);
}

int8_t Semaphore_lock(struct Semaphore_t* o)
{
    int8_t val;

    CRITICAL_ENTER();
    {
        val = (o->Count > 0);
        if(val != 0)
        {
            o->Count = (int8_t)(o->Count - 1);
        }
    }
    CRITICAL_EXIT();

    return val;
}

int8_t Semaphore_unlock(struct Semaphore_t* o)
{
    int8_t val;

    CRITICAL_ENTER();
    {
        val = (o->Count < o->Max);

        if(val != 0)
        {
            o->Count = (int8_t)(o->Count + 1);

            OS_schedulerLock();

            if(o->Event.ListRead.ListLength != 0)
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

int8_t Semaphore_lockPend(struct Semaphore_t* o, tick_t ticksToWait)
{
    int8_t val;

    val = Semaphore_lock(o);
    if(val == 0 && ticksToWait != 0U)
    {
        /* Could not lock semaphore. Pend on it. */
        OS_eventPendTask(
                &o->Event.ListRead,
                OS_taskGetCurrentPriority(),
                ticksToWait);
    }

    return val;
}
