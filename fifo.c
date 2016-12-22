/*
 LibreRTOS - Portable single-stack Real Time Operating System.

 Character FIFO. Specialized character queue. Read/write several characters with
 one read/write call.

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

/** Initialize character FIFO.

 @param buff Pointer to the memory buffer the FIFO will use. The memory buffer
 must be at least length bytes long.
 @param length Length of the character FIFO (the number of characters it can
 hold).

 Initialize character FIFO:
 #define FIFOLEN 16
 uint8_t fifoBuffer[FIFOLEN];
 struct Fifo_t fifo;
 Fifo_init(&fifo, fifoBuffer, FIFOLEN);
 */
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

/** Read from character FIFO.

 Remove up to length items from the character FIFO; copy them to the provided
 buffer.

 @param buff Buffer where to write the characters being read (and removed) from
 the character FIFO. Must be at least length bytes long.
 @param length Maximum number of characters to be read from the character FIFO.
 @return Number of characters read from the character FIFO, 0 if it is empty.

 Read from character FIFO:
 #define NUM 3
 uint8_t buff[NUM];
 Fifo_read(&fifo, buff, NUM);
 */
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

/** Write to character FIFO.

 Add up to length characters to the character FIFO, coping them from the
 provided buffer.

 @param buff Buffer from where to read the characters being written to the
 character FIFO. Must be at least length bytes long.
 @param length Maximum number of characters to be written to the character FIFO.
 @return Number of characters written to the character FIFO, 0 if it is full.

 Write to character FIFO:
 #define NUM 3
 uint8_t buff[NUM];
 init_buff(buff);
 Fifo_write(&fifo, buff, NUM);
 */
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

/** Read or pend on character FIFO.

 Try read the character FIFO; pend on it not successful.

 Can be called only by tasks.

 If the task pends it will not run until the character FIFO has least length
 used bytes or the timeout expires.

 @param buff Buffer from where to read the characters being written to the
 character FIFO. Must be at least length bytes long.
 @param length Maximum number of characters to be written to the character FIFO.
 @param ticksToWait Number of ticks the task will wait for the character FIFO
 (timeout). Passing MAX_DELAY the task will not wakeup by timeout.
 @return Number of characters read from the character FIFO, 0 if it is empty.

 Read or pend on character FIFO without timeout:
 uint8_t buff[LEN];
 Fifo_readPend(&fifo, buff, LEN, MAX_DELAY);

 Read or pend on character FIFO with timeout of 10 ticks:
 uint8_t buff[LEN];
 Fifo_readPend(&fifo, buff, LEN, 10);
 */
uint8_t Fifo_readPend(struct Fifo_t* o, void* buff, uint8_t length, tick_t ticksToWait)
{
    uint8_t val = Fifo_read(o, buff, length);
    if(val == 0)
        Fifo_pendRead(o, length, ticksToWait);
    return val;
}

/** Write or pend on character FIFO.

 Try write the character FIFO; pend on it not successful.

 Can be called only by tasks.

 If the task pends it will not run until the character FIFO has at least length
 free bytes or the timeout expires.

 @param buff Buffer from where to read the characters being written to the
 character FIFO. Must be at least length bytes long.
 @param length Maximum number of characters to be written to the character FIFO.
 @param ticksToWait Number of ticks the task will wait for the character FIFO
 (timeout). Passing MAX_DELAY the task will not wakeup by timeout.
 @return Number of characters written to the character FIFO, 0 if it is full.

 Write or pend on character FIFO without timeout:
 uint8_t buff[LEN];
 init_buff(buff);
 Fifo_writePend(&fifo, buff, LEN, MAX_DELAY);

 Write or pend on character FIFO with timeout of 10 ticks:
 uint8_t buff[LEN];
 init_buff(buff);
 Fifo_writePend(&fifo, buff, LEN, 10);
 */
uint8_t Fifo_writePend(struct Fifo_t* o, const void* buff, uint8_t length, tick_t ticksToWait)
{
    uint8_t val = Fifo_write(o, buff, length);
    if(val == 0)
        Fifo_pendWrite(o, length, ticksToWait);
    return val;
}

/** Pend on character FIFO waiting to read.

 Can be called only by tasks.

 The task will not run until the character FIFO has least length
 used bytes or the timeout expires.

 @param length Maximum number of characters to be written to the character FIFO.
 @param ticksToWait Number of ticks the task will wait for the character FIFO
 (timeout). Passing MAX_DELAY the task will not wakeup by timeout.

 Pend on character FIFO waiting to read without timeout:
 Fifo_pendRead(&fifo, LEN, MAX_DELAY);

 Pend on character FIFO waiting to read with timeout of 10 ticks:
 Fifo_pendRead(&fifo, LEN, 10);
 */
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

/** Pend on character FIFO waiting to write.

 Can be called only by tasks.

 The task will not run until the character FIFO has at least length
 free bytes or the timeout expires.

 @param length Maximum number of characters to be written to the character FIFO.
 @param ticksToWait Number of ticks the task will wait for the character FIFO
 (timeout). Passing MAX_DELAY the task will not wakeup by timeout.

 Pend on character FIFO waiting to write:
 Fifo_pendWrite(&fifo,, LEN, MAX_DELAY);

 Pend on character FIFO waiting to write:
 Fifo_pendWrite(&fifo,, LEN, 10);
 */
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

/** Get number of used items on a character FIFO.

 A character FIFO can be read if there is one or more used items.

 @return Number of used items.

 Get number of used items on a character FIFO:
 Fifo_used(&fifo)
 */
uint8_t Fifo_used(const struct Fifo_t *o)
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

/** Get number of free items on a character FIFO.

 A character FIFO can be written if there is one or more free items.

 @return Number of free items.

 Get number of free items on a character FIFO:
 Fifo_free(&fifo)
 */
uint8_t Fifo_free(const struct Fifo_t *o)
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

/** Get character FIFO length.

 The character FIFO length is the total number of characters it can hold.

 @return Character FIFO length.

 Get length of a character FIFO:
 Fifo_length(&fifo)
 */
uint8_t Fifo_length(const struct Fifo_t *o)
{
    /* This value is constant after initialization. No need for locks. */
    return o->Length;
}
