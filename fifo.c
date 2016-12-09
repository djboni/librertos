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
#include <string.h>

void Fifo_init(
        struct Fifo_t *o,
        void *buff,
        uint8_t length)
{
    uint8_t *buff8 = (uint8_t*)buff;

    o->Length = length;
    o->Free = length;
    o->Used = 0U;
    o->WLock = 0U;
    o->RLock = 0U;
    o->Head = buff8;
    o->Tail = buff8;
    o->Buff = buff8;
    o->BufEnd = &buff8[length - 1];
    OS_eventRwInit(&o->Event);
}

uint8_t Fifo_read(struct Fifo_t* o, void* buff, uint8_t length)
{
    /* Pop front */
    uint8_t val;
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        val = (o->Used >= length) ? length : o->Used;
        if(val != 0U)
        {
            uint8_t *pos;
            uint8_t numFromBegin;
            uint8_t lock;

            length = val;

            pos = o->Head;
            numFromBegin = 0;
            if((o->Head += length) > o->BufEnd)
            {
                numFromBegin = (uint8_t)((pos + length) - (o->BufEnd + 1));
                length = (uint8_t)(length - numFromBegin);
                o->Head -= o->Length;
            }

            lock = o->RLock;
            o->RLock = (uint8_t)(o->RLock + val);
            o->Used = (uint8_t)(o->Used - val);

            CRITICAL_EXIT();
            {
                memcpy(buff, pos, length);
                if(numFromBegin != 0)
                memcpy((uint8_t*)buff + length, o->Buff, numFromBegin);
            }
            CRITICAL_ENTER();

            if(lock == 0U)
            {
                o->Free = (uint8_t)(o->Free + o->RLock);
                o->RLock = 0U;
            }

            OS_schedulerLock();

            if(o->Event.ListWrite.Length != 0)
            {
                /* Unblock task waiting to write to this event. */
                struct taskListNode_t* node = o->Event.ListWrite.Tail;

                /* Length waiting for. */
                if((uint8_t)node->Value <= o->Free)
                    OS_eventUnblockTasks(&(o->Event.ListWrite));
            }
        }
    }
    CRITICAL_EXIT();

    if(val != 0)
        OS_schedulerUnlock();

    return val;
}

uint8_t Fifo_write(struct Fifo_t* o, const void* buff, uint8_t length)
{
    /* Push back */
    uint8_t val;
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        val = (o->Free >= length) ? length : o->Free;
        if(val != 0U)
        {
            uint8_t *pos;
            uint8_t numFromBegin;
            uint8_t lock;

            length = val;

            pos = o->Tail;
            numFromBegin = 0;
            if((o->Tail += length) > o->BufEnd)
            {
                numFromBegin = (uint8_t)((pos + length) - (o->BufEnd + 1));
                length = (uint8_t)(length - numFromBegin);
                o->Tail -= o->Length;
            }

            lock = o->WLock;
            o->WLock = (uint8_t)(o->WLock + val);
            o->Free = (uint8_t)(o->Free - val);

            OS_schedulerLock();

            CRITICAL_EXIT();
            {
                memcpy(pos, buff, length);
                if(numFromBegin != 0)
                    memcpy(o->Buff, (uint8_t*)buff + length, numFromBegin);
            }
            CRITICAL_ENTER();

            if(lock == 0U)
            {
                o->Used = (uint8_t)(o->Used + o->WLock);
                o->WLock = 0U;
            }

            if(o->Event.ListRead.Length != 0)
            {
                /* Unblock task waiting to read from this event. */
                struct taskListNode_t* node = o->Event.ListRead.Tail;

                /* Length waiting for. */
                if((uint8_t)node->Value <= o->Used)
                    OS_eventUnblockTasks(&(o->Event.ListRead));
            }
        }
    }
    CRITICAL_EXIT();

    if(val != 0)
        OS_schedulerUnlock();

    return val;
}

uint8_t Fifo_readPend(struct Fifo_t* o, void* buff, uint8_t length, tick_t ticksToWait)
{
    uint8_t val = Fifo_read(o, buff, length);
    if(val == 0)
        Fifo_pendRead(o, length, ticksToWait);
    return val;
}

uint8_t Fifo_writePend(struct Fifo_t* o, const void* buff, uint8_t length, tick_t ticksToWait)
{
    uint8_t val = Fifo_write(o, buff, length);
    if(val == 0)
        Fifo_pendWrite(o, length, ticksToWait);
    return val;
}

void Fifo_pendRead(struct Fifo_t* o, uint8_t length, tick_t ticksToWait)
{
    if(ticksToWait != 0U)
    {
        struct task_t* task = OS_getCurrentTask();
        CRITICAL_VAL();

        OS_schedulerLock();
        CRITICAL_ENTER();
        if(o->Used < length)
        {
            task->NodeEvent.Value = length; /* Length waiting for. */
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

void Fifo_pendWrite(struct Fifo_t* o, uint8_t length, tick_t ticksToWait)
{
    if(ticksToWait != 0U)
    {
        struct task_t* task = OS_getCurrentTask();
        CRITICAL_VAL();

        OS_schedulerLock();
        CRITICAL_ENTER();
        if(o->Free < length)
        {
            task->NodeEvent.Value = length; /* Length waiting for. */
            OS_eventPrePendTask(&o->Event.ListWrite, task);
            CRITICAL_EXIT();
            OS_eventPendTask(&o->Event.ListWrite, task, ticksToWait);
        }
        else
        {
            CRITICAL_EXIT();
        }
        OS_schedulerUnlock();
    }
}

uint8_t Fifo_used(const struct Fifo_t *o)
{
    return o->Used;
}

uint8_t Fifo_free(const struct Fifo_t *o)
{
    return o->Free;
}
