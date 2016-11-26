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

void Semaphore_init(struct Semaphore_t* o, uint8_t count)
{
    o->Count = count;
    OS_eventRInit(&o->Event);
}

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

void Semaphore_give(struct Semaphore_t* o)
{
    CRITICAL_VAL();

	OS_schedulerLock();

	CRITICAL_ENTER();
    {
		o->Count = (uint8_t)(o->Count + 1);

		if(o->Event.ListRead.ListLength != 0)
		{
			/* Unblock tasks waiting to read from this event. */
			OS_eventUnblockTasks(&(o->Event.ListRead));
		}
    }
    CRITICAL_EXIT();

	OS_schedulerUnlock();
}

uint8_t Semaphore_takePend(struct Semaphore_t* o, tick_t ticksToWait)
{
    uint8_t val = Semaphore_take(o);
    if(val == 0)
        Semaphore_pend(o, ticksToWait);
    return val;
}

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

uint8_t Semaphore_getCount(struct Semaphore_t* o)
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
