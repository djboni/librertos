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

void Queue_init(
        struct Queue_t *o,
        void *buff,
        int8_t length,
        int8_t item_size)
{
    uint8_t *buff8 = (uint8_t*)buff;

    UNCONST(int8_t, o->ItemSize) = item_size;
    o->Free = length;
    o->Used = 0U;
    o->WLock = 0U;
    o->RLock = 0U;
    o->Head = buff8;
    o->Tail = buff8;
    UNCONST(uint8_t*, o->Buff) = buff8;
    UNCONST(uint8_t*, o->BufEnd) = &buff8[(length - 1) * item_size];
    OS_eventRwInit(&o->Event);
}

int8_t Queue_read(struct Queue_t* o, void* buff)
{
    /* Pop front */
    int8_t val;
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        val = (o->Used != 0U);
        if(val != 0U)
        {
            int8_t lock;
            uint8_t *pos;

            lock = (o->RLock)++;
            --(o->Used);

            pos = o->Head;
            if((o->Head += o->ItemSize) > o->BufEnd)
                o->Head = o->Buff;

            CRITICAL_EXIT();
            {
                memcpy(buff, pos, (uint8_t)o->ItemSize);
            }
            CRITICAL_ENTER();

            if(lock == 0U)
            {
                o->Free = (int8_t)(o->Free + o->RLock);
                o->RLock = 0U;
            }

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

int8_t Queue_write(struct Queue_t* o, const void* buff)
{
    /* Push back */
    int8_t val;
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        val = (o->Free != 0U);
        if(val != 0U)
        {
            int8_t lock;
            uint8_t *pos;

            lock = (o->WLock)++;
            --(o->Free);

            pos = o->Tail;
            if((o->Tail += o->ItemSize) > o->BufEnd)
                o->Tail = o->Buff;

            CRITICAL_EXIT();
            {
                memcpy(pos, buff, (uint8_t)o->ItemSize);
            }
            CRITICAL_ENTER();

            if(lock == 0U)
            {
                o->Used = (int8_t)(o->Used + o->WLock);
                o->WLock = 0U;
            }

            OS_schedulerLock();

            if(o->Event.ListWrite.ListLength != 0)
            {
                /* Unblock tasks waiting to write to this event. */
                OS_eventUnblockTasks(&(o->Event.ListWrite));
            }
        }
    }
    CRITICAL_EXIT();

    if(val != 0)
        OS_schedulerUnlock();

    return val;
}

int8_t Queue_readPend(struct Queue_t* o, void* buff, tick_t ticksToWait)
{
    int8_t val;

    val = Queue_read(o, buff);
    if(val == 0 && ticksToWait != 0U)
    {
        /* Could read from queue. Pend on it. */
        OS_eventPendTask(
                &o->Event.ListRead,
                OS_getCurrentPriority(),
                ticksToWait);
    }

    return val;
}

int8_t Queue_writePend(struct Queue_t* o, const void* buff, tick_t ticksToWait)
{
    int8_t val;

    val = Queue_write(o, buff);
    if(val == 0 && ticksToWait != 0U)
    {
        /* Could not wrote to queue. Pend on it. */
        OS_eventPendTask(
                &o->Event.ListWrite,
                OS_getCurrentPriority(),
                ticksToWait);
    }

    return val;
}

int8_t Queue_used(const struct Queue_t *o)
{
    return o->Used;
}

int8_t Queue_free(const struct Queue_t *o)
{
    return o->Free;
}
