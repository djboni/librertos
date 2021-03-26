/*
 LibreRTOS - Portable single-stack Real Time Operating System.

 Queue. Variable length data queue.

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

#include "librertos.h"
#include "librertos_impl.h"

extern void *memcpy(void *dest_ptr, const void *src_ptr, size_t num);

/** Initialize queue.

 @param buff Pointer to the memory buffer the queue will use. Must be at least
 length * item_size bytes long.
 @param length Length of the queue (the number of items it can hold).
 @param item_size Size of each queue item.

 Initialize queue:
 #define QUELEN 4
 #define QUEISZ 16
 uint8_t queBuffer[QUELEN * QUEISZ];
 struct queue_t que;
 Queue_init(&que, queBuffer, QUELEN, QUEISZ);
 */
void QueueInit(struct queue_t *ptr, void *buff_ptr, len_t length,
               len_t item_size) {
  uint8_t *buff8_ptr = (uint8_t *)buff_ptr;

  ptr->item_size = item_size;
  ptr->free = length;
  ptr->used = 0U;
  ptr->w_lock = 0U;
  ptr->r_lock = 0U;
  ptr->head_ptr = buff8_ptr;
  ptr->tail_ptr = buff8_ptr;
  ptr->buff_ptr = buff8_ptr;
  ptr->buff_end_ptr = &buff8_ptr[(length - 1) * item_size];
  OSEventRwInit(&ptr->event);
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
bool_t QueueRead(struct queue_t *ptr, void *buff_ptr) {
  /* Pop front */
  bool_t val;
  CRITICAL_VAL();

  CRITICAL_ENTER();
  {
    val = (ptr->used != 0U);
    if (val != 0U) {
      uint8_t *pos_ptr;
      len_t lock;

      pos_ptr = ptr->head_ptr;
      if ((ptr->head_ptr += ptr->item_size) > ptr->buff_end_ptr) {
        ptr->head_ptr = ptr->buff_ptr;
      }

      lock = (ptr->r_lock)++;
      --(ptr->used);

      CRITICAL_EXIT();
      {
        memcpy(buff_ptr, pos_ptr, (size_t)ptr->item_size);

        /* For test coverage only. This macro is used as a deterministic
         way to create a concurrent access. */
        LIBRERTOS_TEST_CONCURRENT_ACCESS();
      }
      CRITICAL_ENTER();

      if (lock == 0U) {
        ptr->free = (len_t)(ptr->free + ptr->r_lock);
        ptr->r_lock = 0U;
      }

      SchedulerLock();

      if (ptr->event.list_write.length != 0) {
        /* Unblock tasks waiting to write to this event. */
        OSEventUnblockTasks(&(ptr->event.list_write));
      }
    }
  }
  CRITICAL_EXIT();

  if (val != 0) {
    SchedulerUnlock();
  }

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
bool_t QueueWrite(struct queue_t *ptr, const void *buff_ptr) {
  /* Push back */
  bool_t val;
  CRITICAL_VAL();

  CRITICAL_ENTER();
  {
    val = (ptr->free != 0U);
    if (val != 0U) {
      uint8_t *pos_ptr;
      len_t lock;

      pos_ptr = ptr->tail_ptr;
      if ((ptr->tail_ptr += ptr->item_size) > ptr->buff_end_ptr) {
        ptr->tail_ptr = ptr->buff_ptr;
      }

      lock = (ptr->w_lock)++;
      --(ptr->free);

      SchedulerLock();

      CRITICAL_EXIT();
      {
        memcpy(pos_ptr, buff_ptr, (size_t)ptr->item_size);

        /* For test coverage only. This macro is used as a deterministic
         way to create a concurrent access. */
        LIBRERTOS_TEST_CONCURRENT_ACCESS();
      }
      CRITICAL_ENTER();

      if (lock == 0U) {
        ptr->used = (len_t)(ptr->used + ptr->w_lock);
        ptr->w_lock = 0U;
      }

      if (ptr->event.list_read.length != 0) {
        /* Unblock tasks waiting to read from this event. */
        OSEventUnblockTasks(&(ptr->event.list_read));
      }
    }
  }
  CRITICAL_EXIT();

  if (val != 0) {
    SchedulerUnlock();
  }

  return val;
}

/** Read or pend on queue.

 Try read the queue; pend on it not successful.

 Can be called only by tasks.

 If the task pends it will not run until the queue is written or the timeout
 expires.

 @param buff Buffer where to write the item being read (and removed) from the
 queue. Must be at least QUEISZ bytes long.
 @param ticks_to_wait Number of ticks the task will wait for the queue
 (timeout). Passing MAX_DELAY the task will not wakeup by timeout.
 @return 1 if success, 0 otherwise.

 Read or pend on queue without timeout:
 uint8_t buff[QUEISZ];
 Queue_readPend(&que, buff, MAX_DELAY);

 Read or pend on queue with timeout of 10 ticks:
 uint8_t buff[QUEISZ];
 Queue_readPend(&que, buff, 10);
 */
bool_t QueueReadPend(struct queue_t *ptr, void *buff_ptr,
                     tick_t ticks_to_wait) {
  bool_t val = QueueRead(ptr, buff_ptr);
  if (val == 0) {
    QueuePendRead(ptr, ticks_to_wait);
  }
  return val;
}

/** Write or pend on queue.

 Try write the queue; pend on it not successful.

 Can be called only by tasks.

 If the task pends it will not run until the queue is read or the timeout
 expires.

 @param buff Buffer from where to read item being written to the queue. Must be
 at least QUEISZ bytes long.
 @param ticks_to_wait Number of ticks the task will wait for the queue
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
bool_t QueueWritePend(struct queue_t *ptr, const void *buff_ptr,
                      tick_t ticks_to_wait) {
  bool_t val = QueueWrite(ptr, buff_ptr);
  if (val == 0) {
    QueuePendWrite(ptr, ticks_to_wait);
  }
  return val;
}

/** Pend on queue waiting to read.

 Can be called only by tasks.

 The task will not run until the queue is written or the timeout expires.

 @param ticks_to_wait Number of ticks the task will wait for the queue
 (timeout). Passing MAX_DELAY the task will not wakeup by timeout.
 @return 1 if success, 0 otherwise.

 Pend on queue waiting to read without timeout:
 Queue_pendRead(&que, MAX_DELAY);

 Pend on queue waiting to read with timeout of 10 ticks:
 Queue_pendRead(&que, 10);
 */
void QueuePendRead(struct queue_t *ptr, tick_t ticks_to_wait) {
  if (ticks_to_wait != 0U) {
    struct task_t *task_ptr = GetCurrentTask();

    SchedulerLock();
    INTERRUPTS_DISABLE();
    if (ptr->used == 0U) {
      OSEventPrePendTask(&ptr->event.list_read, task_ptr);
      INTERRUPTS_ENABLE();
      OSEventPendTask(&ptr->event.list_read, task_ptr, ticks_to_wait);
    } else {
      INTERRUPTS_ENABLE();
    }
    SchedulerUnlock();
  }
}

/** Pend on queue waiting to write.

 Can be called only by tasks.

 The task will not run until the queue is read or the timeout expires.

 @param ticks_to_wait Number of ticks the task will wait for the queue
 (timeout). Passing MAX_DELAY the task will not wakeup by timeout.
 @return 1 if success, 0 otherwise.

 Pend on queue waiting to write without timeout:
 Queue_pendWrite(&que, MAX_DELAY);

 Pend on queue waiting to write with timeout of 10 ticks:
 Queue_pendWrite(&que, 10);
 */
void QueuePendWrite(struct queue_t *ptr, tick_t ticks_to_wait) {
  if (ticks_to_wait != 0U) {
    struct task_t *task_ptr = GetCurrentTask();

    SchedulerLock();
    INTERRUPTS_DISABLE();
    if (ptr->free == 0U) {
      OSEventPrePendTask(&ptr->event.list_write, task_ptr);
      INTERRUPTS_ENABLE();
      OSEventPendTask(&ptr->event.list_write, task_ptr, ticks_to_wait);
    } else {
      INTERRUPTS_ENABLE();
    }
    SchedulerUnlock();
  }
}

/** Get number of used items on a queue.

 A queue can be read if there is one or more used items.

 @return Number of used items.

 Get number of used items on a queue:
 Queue_used(&que)
 */
len_t QueueUsed(const struct queue_t *ptr) {
  len_t val;
  CRITICAL_VAL();
  CRITICAL_ENTER();
  { val = ptr->used; }
  CRITICAL_EXIT();
  return val;
}

/** Get number of free items on a queue.

 A queue can be written if there is one or more free items.

 @return Number of free items.

 Get number of free items on a queue:
 Queue_free(&que)
 */
len_t QueueFree(const struct queue_t *ptr) {
  len_t val;
  CRITICAL_VAL();
  CRITICAL_ENTER();
  { val = ptr->free; }
  CRITICAL_EXIT();
  return val;
}

/** Get queue length.

 The queue length is the total number of items it can hold.

 @return Queue length.

 Get length of a queue:
 Queue_length(&que)
 */
len_t QueuLength(const struct queue_t *ptr) {
  len_t val;
  CRITICAL_VAL();
  CRITICAL_ENTER();
  { val = (len_t)(ptr->free + ptr->used + ptr->w_lock + ptr->r_lock); }
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
len_t QueueItemSize(const struct queue_t *ptr) {
  /* This value is constant after initialization. No need for locks. */
  return ptr->item_size;
}

bool_t QueueEmpty(const struct queue_t *ptr) { return QueueUsed(ptr) == 0; }

bool_t QueueFull(const struct queue_t *ptr) { return QueueFree(ptr) == 0; }
