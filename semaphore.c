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

void Semaphore_init(struct Semaphore_t* o, int8_t count)
{
    o->Count = count;
    OS_eventRInit(&o->Event);
}

int8_t Semaphore_take(struct Semaphore_t* o)
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

void Semaphore_give(struct Semaphore_t* o)
{
	OS_schedulerLock();

	CRITICAL_ENTER();
    {
		o->Count = (int8_t)(o->Count + 1);

		if(o->Event.ListRead.ListLength != 0)
		{
			/* Unblock tasks waiting to read from this event. */
			OS_eventUnblockTasks(&(o->Event.ListRead));
		}
    }
    CRITICAL_EXIT();

	OS_schedulerUnlock();
}

int8_t Semaphore_takePend(struct Semaphore_t* o, tick_t ticksToWait)
{
    int8_t val;

    val = Semaphore_take(o);
    if(val == 0 && ticksToWait != 0U)
    {
        /* Could not lock semaphore. Pend on it. */
    	priority_t priority = OS_getCurrentPriority();
        OS_eventPendTask(
                &o->Event.ListRead,
				priority,
                ticksToWait);
    }

    return val;
}

int8_t Semaphore_getCount(struct Semaphore_t* o)
{
    int8_t val;
    CRITICAL_ENTER();
    {
        val = o->Count;
    }
    CRITICAL_EXIT();
    return val;
}
