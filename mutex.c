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

#define MUTEX_NOT_OWNED ((struct task_t*)NULL)

void Mutex_init(struct Mutex_t* o)
{
    o->Count = 0;
    o->MutexOwner = MUTEX_NOT_OWNED;
    OS_eventRInit(&o->Event);
}

uint8_t Mutex_lock(struct Mutex_t* o)
{
    uint8_t val;
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
    	struct task_t* currentTask = OS_getCurrentTask();
        val = o->Count == 0 || o->MutexOwner == currentTask;
        if(val != 0)
        {
            o->Count = (uint8_t)(o->Count + 1);
            o->MutexOwner = currentTask;
        }
    }
    CRITICAL_EXIT();

    return val;
}

void Mutex_unlock(struct Mutex_t* o)
{
    CRITICAL_VAL();

	OS_schedulerLock();

	CRITICAL_ENTER();
    {
		o->Count = (uint8_t)(o->Count - 1);

		if(		o->Count == 0 &&
				o->Event.ListRead.ListLength != 0)
		{
			/* Unblock tasks waiting to read from this event. */
			OS_eventUnblockTasks(&(o->Event.ListRead));
		}
    }
    CRITICAL_EXIT();

	OS_schedulerUnlock();
}

uint8_t Mutex_lockPend(struct Mutex_t* o, tick_t ticksToWait)
{
    uint8_t val = Mutex_lock(o);
    if(val == 0)
        Mutex_pend(o, ticksToWait);
    return val;
}

void Mutex_pend(struct Mutex_t* o, tick_t ticksToWait)
{
    if(ticksToWait != 0U)
    {
        struct task_t* task = OS_getCurrentTask();
        CRITICAL_VAL();

        OS_schedulerLock();
        CRITICAL_ENTER();
        if(o->Count != 0 && o->MutexOwner != task)
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
