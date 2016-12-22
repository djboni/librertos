/*
 LibreRTOS - Portable single-stack Real Time Operating System.

 Queue. Variable length data queue.

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

/** Initialize queue.

 @param buff Pointer to the memory buffer the queue will use. Must be at least
 length * item_size bytes long.
 @param length Length of the queue (the number of items it can hold).
 @param item_size Size of each queue item.

 Initialize queue:
 #define QUELEN 4
 #define QUEISZ 16
 uint8_t queBuffer[QUELEN * QUEISZ];
 struct Queue_t que;
 Queue_init(&que, queBuffer, QUELEN, QUEISZ);
 */
void Queue_init(
        struct Queue_t *o,
        void *buff,
        uint8_t length,
        uint8_t item_size)
{
    uint8_t *buff8 = (uint8_t*)buff;

    o->ItemSize = item_size;
    o->Free = length;
    o->Used = 0U;
    o->WLock = 0U;
    o->RLock = 0U;
    o->Head = buff8;
    o->Tail = buff8;
    o->Buff = buff8;
    o->BufEnd = &buff8[(length - 1) * item_size];
    OS_eventRwInit(&o->Event);
}

/** Read item from queue.

 Remove one item from the queue; copy it to the provided buffer.

 @param buff Buffer where to write the item being read (and removed) from the
 queue. Must be at least QUEISZ bytes long.
 @return 1 if success, 0 otherwise.

 Read item from queue:
 uint8_t buff[QUEISZ];
 Queue_read(&que, buff);
 */
uint8_t Queue_read(struct Queue_t* o, void* buff)
{
    /* Pop front */
    uint8_t val;
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        val = (o->Used != 0U);
        if(val != 0U)
        {
            uint8_t *pos;
            uint8_t lock;

            pos = o->Head;
            if((o->Head += o->ItemSize) > o->BufEnd)
                o->Head = o->Buff;

            lock = (o->RLock)++;
            --(o->Used);

            CRITICAL_EXIT();
            {
                memcpy(buff, pos, (uint8_t)o->ItemSize);

                /* For test coverage only. This macro is used as a deterministic
                 way to create a concurrent access. */
                LIBRERTOS_TEST_CONCURRENT_ACCESS();
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

/** Write item to queue.

 Add one item to the queue, coping it from the provided buffer.

 @param buff Buffer from where to read item being written to the queue. Must be
 at least QUEISZ bytes long.
 @return 1 if success, 0 otherwise.

 Write item to queue:
 uint8_t buff[QUEISZ];
 init_buff(buff);
 Queue_write(&que, buff);
 */
uint8_t Queue_write(struct Queue_t* o, const void* buff)
{
    /* Push back */
    uint8_t val;
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        val = (o->Free != 0U);
        if(val != 0U)
        {
            uint8_t *pos;
            uint8_t lock;

            pos = o->Tail;
            if((o->Tail += o->ItemSize) > o->BufEnd)
                o->Tail = o->Buff;

            lock = (o->WLock)++;
            --(o->Free);

            OS_schedulerLock();

            CRITICAL_EXIT();
            {
                memcpy(pos, buff, (uint8_t)o->ItemSize);

                /* For test coverage only. This macro is used as a deterministic
                 way to create a concurrent access. */
                LIBRERTOS_TEST_CONCURRENT_ACCESS();
            }
            CRITICAL_ENTER();

            if(lock == 0U)
            {
                o->Used = (uint8_t)(o->Used + o->WLock);
                o->WLock = 0U;
            }

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

/** Read or pend on queue.

 Try read the queue; pend on it not successful.

 Can be called only by tasks.

 If the task pends it will not run until the queue is written or the timeout
 expires.

 @param buff Buffer where to write the item being read (and removed) from the
 queue. Must be at least QUEISZ bytes long.
 @param ticksToWait Number of ticks the task will wait for the queue
 (timeout). Passing MAX_DELAY the task will not wakeup by timeout.
 @return 1 if success, 0 otherwise.

 Read or pend on queue without timeout:
 uint8_t buff[QUEISZ];
 Queue_readPend(&que, buff, MAX_DELAY);

 Read or pend on queue with timeout of 10 ticks:
 uint8_t buff[QUEISZ];
 Queue_readPend(&que, buff, 10);
 */
uint8_t Queue_readPend(struct Queue_t* o, void* buff, tick_t ticksToWait)
{
    uint8_t val = Queue_read(o, buff);
    if(val == 0)
        Queue_pendRead(o, ticksToWait);
    return val;
}

/** Write or pend on queue.

 Try write the queue; pend on it not successful.

 Can be called only by tasks.

 If the task pends it will not run until the queue is read or the timeout
 expires.

 @param buff Buffer from where to read item being written to the queue. Must be
 at least QUEISZ bytes long.
 @param ticksToWait Number of ticks the task will wait for the queue
 (timeout). Passing MAX_DELAY the task will not wakeup by timeout.
 @return 1 if success, 0 otherwise.

 Write or pend on queue without timeout:
 uint8_t buff[QUEISZ];
 init_buff(buff);
 Queue_writePend(&que, buff, MAX_DELAY);

 Write or pend on queue with timeout of 10 ticks:
 uint8_t buff[QUEISZ];
 init_buff(buff);
 Queue_writePend(&que, buff, 10);
 */
uint8_t Queue_writePend(struct Queue_t* o, const void* buff, tick_t ticksToWait)
{
    uint8_t val = Queue_write(o, buff);
    if(val == 0)
        Queue_pendWrite(o, ticksToWait);
    return val;
}

/** Pend on queue waiting to read.

 Can be called only by tasks.

 The task will not run until the queue is written or the timeout expires.

 @param ticksToWait Number of ticks the task will wait for the queue
 (timeout). Passing MAX_DELAY the task will not wakeup by timeout.
 @return 1 if success, 0 otherwise.

 Pend on queue waiting to read without timeout:
 Queue_pendRead(&que, MAX_DELAY);

 Pend on queue waiting to read with timeout of 10 ticks:
 Queue_pendRead(&que, 10);
 */
void Queue_pendRead(struct Queue_t* o, tick_t ticksToWait)
{
    if(ticksToWait != 0U)
    {
        struct task_t* task = OS_getCurrentTask();
        CRITICAL_VAL();

        OS_schedulerLock();
        CRITICAL_ENTER();
        if(o->Used == 0U)
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

/** Pend on queue waiting to write.

 Can be called only by tasks.

 The task will not run until the queue is read or the timeout expires.

 @param ticksToWait Number of ticks the task will wait for the queue
 (timeout). Passing MAX_DELAY the task will not wakeup by timeout.
 @return 1 if success, 0 otherwise.

 Pend on queue waiting to write without timeout:
 Queue_pendWrite(&que, MAX_DELAY);

 Pend on queue waiting to write with timeout of 10 ticks:
 Queue_pendWrite(&que, 10);
 */
void Queue_pendWrite(struct Queue_t* o, tick_t ticksToWait)
{
    if(ticksToWait != 0U)
    {
        struct task_t* task = OS_getCurrentTask();
        CRITICAL_VAL();

        OS_schedulerLock();
        CRITICAL_ENTER();
        if(o->Free == 0U)
        {
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

/** Get number of used items on a queue.

 A queue can be read if there is one or more used items.

 @return Number of used items.

 Get number of used items on a queue:
 Queue_used(&que)
 */
uint8_t Queue_used(const struct Queue_t *o)
{
    uint8_t val;
    CRITICAL_VAL();
    CRITICAL_ENTER();
    {
        val = o->Used;
    }
    CRITICAL_EXIT();
    return val;
}

/** Get number of free items on a queue.

 A queue can be written if there is one or more free items.

 @return Number of free items.

 Get number of free items on a queue:
 Queue_free(&que)
 */
uint8_t Queue_free(const struct Queue_t *o)
{
    uint8_t val;
    CRITICAL_VAL();
    CRITICAL_ENTER();
    {
        val = o->Free;
    }
    CRITICAL_EXIT();
    return val;
}

/** Get queue length.

 The queue length is the total number of items it can hold.

 @return Queue length.

 Get length of a queue:
 Queue_length(&que)
 */
uint8_t Queue_length(const struct Queue_t *o)
{
    uint8_t val;
    CRITICAL_VAL();
    CRITICAL_ENTER();
    {
        val = (uint8_t)(o->Free + o->Used + o->WLock + o->RLock);
    }
    CRITICAL_EXIT();
    return val;
}

/** Get queue item size.

 The queue item size is the size of the items that are inserted into and removed
 from the queue.

 @return Queue item size.

 Get queue item size:
 Queue_itemSize(&que)
 */
uint8_t Queue_itemSize(const struct Queue_t *o)
{
    /* This value is constant after initialization. No need for locks. */
    return o->ItemSize;
}
