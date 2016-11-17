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

#define UNCONST(type, var) *((type*)&(var))

void Fifo_init(
        struct Fifo_t *o,
        void *buff,
        int8_t length)
{
    uint8_t *buff8 = (uint8_t*)buff;

    UNCONST(int8_t, o->Length) = length;
    o->Free = length;
    o->Used = 0U;
    #if (LIBRERTOS_FIFO_1CRITICAL == 0)
        o->WLock = 0U;
        o->RLock = 0U;
    #endif
    o->Head = buff8;
    o->Tail = buff8;
    UNCONST(uint8_t*, o->Buff) = buff8;
    UNCONST(uint8_t*, o->BufEnd) = &buff8[length - 1];
    OS_eventRwInit(&o->Event);
}

int8_t Fifo_read(struct Fifo_t* o, void* buff, int8_t length)
{
    /* Pop front */
    int8_t val;
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        val = (o->Used >= length) ? length : o->Used;
        if(val != 0U)
        {
            uint8_t *pos;
            int8_t numFromBegin;

            length = val;

            pos = o->Head;
            numFromBegin = 0;
            if((o->Head += length) > o->BufEnd)
            {
                numFromBegin = (pos + length) - (o->BufEnd + 1);
                length -= numFromBegin;
                o->Head -= o->Length;
            }

            #if (LIBRERTOS_FIFO_1CRITICAL == 0)
            {
                int8_t lock;

                lock = o->RLock;
                o->RLock += length;
                o->Used -= length;

                CRITICAL_EXIT();
                {
                    memcpy(buff, pos, length);
                    if(numFromBegin != 0)
                    memcpy((uint8_t*)buff + length, o->Buff, numFromBegin);
                }
                CRITICAL_ENTER();

                if(lock == 0U)
                {
                    o->Free = (int8_t)(o->Free + o->RLock);
                    o->RLock = 0U;
                }
            }
            #else
            {
                o->Used -= val;
                o->Free += val;
                memcpy(buff, pos, length);
                if(numFromBegin != 0)
                    memcpy((uint8_t*)buff + length, o->Buff, numFromBegin);
            }
            #endif

            OS_schedulerLock();

            if(o->Event.ListWrite.ListLength != 0)
            {
                /* Unblock task waiting to write to this event. */
                struct taskListNode_t* node = o->Event.ListWrite.ListTail;

                /* Length waiting for. */
                if((int8_t)node->TickToWakeup <= o->Free)
                    OS_eventUnblockTasks(&(o->Event.ListWrite));
            }
        }
    }
    CRITICAL_EXIT();

    if(val != 0)
        OS_schedulerUnlock();

    return val;
}

int8_t Fifo_write(struct Fifo_t* o, const void* buff, int8_t length)
{
    /* Push back */
    int8_t val;
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        val = (o->Free >= length) ? length : o->Free;
        if(val != 0U)
        {
            uint8_t *pos;
            int8_t numFromBegin;

            length = val;

            pos = o->Tail;
            numFromBegin = 0;
            if((o->Tail += length) > o->BufEnd)
            {
                numFromBegin = (pos + length) - (o->BufEnd + 1);
                length -= numFromBegin;
                o->Tail -= o->Length;
            }

            #if (LIBRERTOS_FIFO_1CRITICAL == 0)
            {
                int8_t lock;

                lock = o->WLock;
                o->WLock += length;
                o->Free -= length;

                CRITICAL_EXIT();
                {
                    memcpy(pos, buff, length);
                    if(numFromBegin != 0)
                        memcpy(o->Buff, (uint8_t*)buff + length, numFromBegin);
                }
                CRITICAL_ENTER();

                if(lock == 0U)
                {
                    o->Used = (int8_t)(o->Used + o->WLock);
                    o->WLock = 0U;
                }
            }
            #else
            {
                o->Free -= val;
                o->Used += val;
                memcpy(pos, buff, length);
                if(numFromBegin != 0)
                    memcpy(o->Buff, (uint8_t*)buff + length, numFromBegin);
            }
            #endif

            OS_schedulerLock();

            if(o->Event.ListRead.ListLength != 0)
            {
                /* Unblock task waiting to read from this event. */
                struct taskListNode_t* node = o->Event.ListRead.ListTail;

                /* Length waiting for. */
                if((int8_t)node->TickToWakeup <= o->Used)
                    OS_eventUnblockTasks(&(o->Event.ListRead));
            }
        }
    }
    CRITICAL_EXIT();

    if(val != 0)
        OS_schedulerUnlock();

    return val;
}

int8_t Fifo_readPend(struct Fifo_t* o, void* buff, int8_t length, tick_t ticksToWait)
{
    int8_t val;

    val = Fifo_read(o, buff, length);
    if(val == 0 && ticksToWait != 0U)
    {
        /* Could read from FIFO. Pend on it. */

        priority_t priority = OS_getCurrentPriority();
        struct taskListNode_t* node = OS_getTaskEventNode(priority);

        /* Length waiting for. */
        node->TickToWakeup = length;

        OS_eventPendTask(
                &o->Event.ListRead,
                priority,
                ticksToWait);
    }

    return val;
}

int8_t Fifo_writePend(struct Fifo_t* o, const void* buff, int8_t length, tick_t ticksToWait)
{
    int8_t val;

    val = Fifo_write(o, buff, length);
    if(val == 0 && ticksToWait != 0U)
    {
        /* Could not write to FIFO. Pend on it. */

        priority_t priority = OS_getCurrentPriority();
        struct taskListNode_t* node = OS_getTaskEventNode(priority);

        /* Length waiting for. */
        node->TickToWakeup = length;

        OS_eventPendTask(
                &o->Event.ListWrite,
                priority,
                ticksToWait);
    }

    return val;
}

int8_t Fifo_used(const struct Fifo_t *o)
{
    return o->Used;
}

int8_t Fifo_free(const struct Fifo_t *o)
{
    return o->Free;
}
