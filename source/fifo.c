/*
 LibreRTOS - Portable single-stack Real Time Operating System.

 Character FIFO. Specialized character queue. Read/write several characters with
 one read/write call.

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

/** Initialize character FIFO.

 @param buff_ptr Pointer to the memory buffer the FIFO will use. The memory
 buffer must be at least length bytes long.
 @param length length of the character FIFO (the number of characters it can
 hold).

 Initialize character FIFO:

 #define FIFOLEN 16
 uint8_t fifo_buffer[FIFOLEN];
 struct fifo_t fifo;
 FifoInit(&fifo, fifo_buffer, FIFOLEN);
 */
void FifoInit(struct fifo_t *ptr, void *buff_ptr, len_t length) {
  uint8_t *buff8_ptr = (uint8_t *)buff_ptr;

  ptr->length = length;
  ptr->free = length;
  ptr->used = 0U;
  ptr->w_lock = 0U;
  ptr->r_lock = 0U;
  ptr->head_ptr = buff8_ptr;
  ptr->tail_ptr = buff8_ptr;
  ptr->buff_ptr = buff8_ptr;
  ptr->buff_end_ptr = &buff8_ptr[length - 1];
  OSEventRwInit(&ptr->event);
}

/** Read one byte from character FIFO (pop front).

 @param buff_ptr Buffer where to write the character being read (and removed)
 from the character FIFO. Must be at least one byte long.
 @return 1 if one byte was read from the character FIFO, 0 if it was empty.

 Read one byte from character FIFO:

 uint8_t ch;
 if(FifoReadByte(&fifo, &ch)) {
   // Success
 }
 else {
   // Empty
 }
 */
bool_t FifoReadByte(struct fifo_t *ptr, void *buff_ptr) {
  CRITICAL_VAL();

  CRITICAL_ENTER();
  if (ptr->used > 0) {
    *(uint8_t *)buff_ptr = *ptr->head_ptr;

    if ((ptr->head_ptr += 1) > ptr->buff_end_ptr) {
      ptr->head_ptr = ptr->buff_ptr;
    }

    ptr->free = (len_t)(ptr->free + 1);
    ptr->used = (len_t)(ptr->used - 1);

    SchedulerLock();

    if (ptr->event.list_write.length != 0) {
      /* Unblock task waiting to write to this event. */
      struct task_list_node_t *node_ptr = ptr->event.list_write.tail_ptr;

      /* length waiting for. */
      if ((len_t)node_ptr->value <= ptr->free) {
        OSEventUnblockTasks(&(ptr->event.list_write));
      }
    }

    CRITICAL_EXIT();
    SchedulerUnlock();
    return 1;
  } else {
    CRITICAL_EXIT();
    return 0;
  }
}

/** Read from character FIFO (pop front).

 Remove up to length items from the character FIFO; copy them to the buffer.

 @param buff_ptr Buffer where to write the characters being read (and removed)
 from the character FIFO. Must be at least length bytes long.
 @param length Maximum number of characters to be read from the character FIFO.
 @return Number of characters read from the character FIFO, 0 if it is empty.

 Read from character FIFO:

 #define NUM 3
 uint8_t buff[NUM];
 FifoRead(&fifo, buff, NUM) {
   // Success or partial
 }
 else {
   // Empty
 }
 */
len_t FifoRead(struct fifo_t *ptr, void *buff_ptr, len_t length) {
  len_t val;
  CRITICAL_VAL();

  CRITICAL_ENTER();
  {
    val = (ptr->used >= length) ? length : ptr->used;
    if (val != 0U) {
      uint8_t *pos_ptr;
      len_t num_from_begin;
      len_t lock;

      length = val;

      pos_ptr = ptr->head_ptr;
      num_from_begin = 0;
      if ((ptr->head_ptr += length) > ptr->buff_end_ptr) {
        num_from_begin = (len_t)((pos_ptr + length) - (ptr->buff_end_ptr + 1));
        length = (len_t)(length - num_from_begin);
        ptr->head_ptr -= ptr->length;
      }

      lock = ptr->r_lock;
      ptr->r_lock = (len_t)(ptr->r_lock + val);
      ptr->used = (len_t)(ptr->used - val);

      CRITICAL_EXIT();
      {
        memcpy(buff_ptr, pos_ptr, (size_t)length);
        if (num_from_begin != 0) {
          memcpy((uint8_t *)buff_ptr + length, ptr->buff_ptr,
                 (size_t)num_from_begin);
        }

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
        /* Unblock task waiting to write to this event. */
        struct task_list_node_t *node_ptr = ptr->event.list_write.tail_ptr;

        /* length waiting for. */
        if ((len_t)node_ptr->value <= ptr->free) {
          OSEventUnblockTasks(&(ptr->event.list_write));
        }
      }
    }
  }
  CRITICAL_EXIT();

  if (val != 0) {
    SchedulerUnlock();
  }

  return val;
}

/** Write one byte to character FIFO (push back).

 Add up to length characters to the character FIFO, coping them from the buffer.

 @param buff_ptr Buffer from where to read the character being written to the
 character FIFO. Must be at least one byte long.
 @return 1 if one byte was written to the character FIFO, 0 if it was full.

 Write to character FIFO:

 uint8_t ch = 'a';
 if(FifoWriteByte(&fifo, &ch)) {
   // Success or partial
 }
 else {
   // Full
 }
 */
bool_t FifoWriteByte(struct fifo_t *ptr, const void *buff_ptr) {
  CRITICAL_VAL();

  CRITICAL_ENTER();
  if (ptr->free > 0) {
    *ptr->tail_ptr = *(uint8_t *)buff_ptr;

    if ((ptr->tail_ptr += 1) > ptr->buff_end_ptr) {
      ptr->tail_ptr = ptr->buff_ptr;
    }

    ptr->free = (len_t)(ptr->free - 1);
    ptr->used = (len_t)(ptr->used + 1);

    SchedulerLock();

    if (ptr->event.list_read.length != 0) {
      /* Unblock task waiting to read from this event. */
      struct task_list_node_t *node_ptr = ptr->event.list_read.tail_ptr;

      /* length waiting for. */
      if ((len_t)node_ptr->value <= ptr->used) {
        OSEventUnblockTasks(&(ptr->event.list_read));
      }
    }

    CRITICAL_EXIT();
    SchedulerUnlock();
    return 1;
  } else {
    CRITICAL_EXIT();
    return 0;
  }
}

/** Write to character FIFO (push back).

 Add up to length characters to the character FIFO, coping them from the buffer.

 @param buff Buffer from where to read the characters being written to the
 character FIFO. Must be at least length bytes long.
 @param length Maximum number of characters to be written to the character FIFO.
 @return Number of characters written to the character FIFO, 0 if it is full.

 Write to character FIFO:

 #define NUM 3
 uint8_t buff[NUM];
 init_buff(buff);
 if(FifoWrite(&fifo, buff, NUM)) {
   // Success or partial
 }
 else {
   // Full
 }
 */
len_t FifoWrite(struct fifo_t *ptr, const void *buff_ptr, len_t length) {
  len_t val;
  CRITICAL_VAL();

  CRITICAL_ENTER();
  {
    val = (ptr->free >= length) ? length : ptr->free;
    if (val != 0U) {
      uint8_t *pos_ptr;
      len_t num_from_begin;
      len_t lock;

      length = val;

      pos_ptr = ptr->tail_ptr;
      num_from_begin = 0;
      if ((ptr->tail_ptr += length) > ptr->buff_end_ptr) {
        num_from_begin = (len_t)((pos_ptr + length) - (ptr->buff_end_ptr + 1));
        length = (len_t)(length - num_from_begin);
        ptr->tail_ptr -= ptr->length;
      }

      lock = ptr->w_lock;
      ptr->w_lock = (len_t)(ptr->w_lock + val);
      ptr->free = (len_t)(ptr->free - val);

      SchedulerLock();

      CRITICAL_EXIT();
      {
        memcpy(pos_ptr, buff_ptr, (size_t)length);
        if (num_from_begin != 0) {
          memcpy(ptr->buff_ptr, (uint8_t *)buff_ptr + length,
                 (size_t)num_from_begin);
        }

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
        /* Unblock task waiting to read from this event. */
        struct task_list_node_t *node_ptr = ptr->event.list_read.tail_ptr;

        /* length waiting for. */
        if ((len_t)node_ptr->value <= ptr->used) {
          OSEventUnblockTasks(&(ptr->event.list_read));
        }
      }
    }
  }
  CRITICAL_EXIT();

  if (val != 0) {
    SchedulerUnlock();
  }

  return val;
}

/** Read and, if fails, pend on character FIFO.

 Try read the character FIFO; pend on it not successful.

 Can be called only by tasks.

 If the task pends it will not run until the character FIFO has least length
 used bytes or the timeout expires.

 @param buff_ptr Buffer from where to read the characters being written to the
 character FIFO. Must be at least length bytes long.
 @param length Maximum number of characters to be written to the character FIFO.
 @param ticks_to_wait Number of ticks the task will wait for the character FIFO
 (timeout). Passing MAX_DELAY the task will not wakeup by timeout.
 @return Number of characters read from the character FIFO, 0 if it is empty.

 Read or pend on character FIFO without timeout:

 uint8_t buff[LEN];
 if(FifoReadPend(&fifo, buff, LEN, MAX_DELAY)) {
   // Success our partial
 }
 else {
   // Empty
 }

 Read or pend on character FIFO with timeout of 10 ticks:

 uint8_t buff[LEN];
 Fifo_readPend(&fifo, buff, LEN, 10){
   // Success our partial
 }
 else {
   // Empty
 }
 */
len_t FifoReadPend(struct fifo_t *ptr, void *buff_ptr, len_t length,
                   tick_t ticks_to_wait) {
  len_t val = FifoRead(ptr, buff_ptr, length);
  if (val == 0) {
    FifoPendRead(ptr, length, ticks_to_wait);
  }
  return val;
}

/** Write and, if fails, pend on character FIFO.

 Try write the character FIFO; pend on it not successful.

 Can be called only by tasks.

 If the task pends it will not run until the character FIFO has at least length
 free bytes or the timeout expires.

 @param buff_ptr Buffer from where to read the characters being written to the
 character FIFO. Must be at least length bytes long.
 @param length Maximum number of characters to be written to the character FIFO.
 @param ticks_to_wait Number of ticks the task will wait for the character FIFO
 (timeout). Passing MAX_DELAY the task will not wakeup by timeout.
 @return Number of characters written to the character FIFO, 0 if it is full.

 Write or pend on character FIFO without timeout:

 uint8_t buff[LEN];
 init_buff(buff);
 if(FifoWritePend(&fifo, buff, LEN, MAX_DELAY)) {
   // Success or partial
 }
 else {
   // Full
 }

 Write or pend on character FIFO with timeout of 10 ticks:

 uint8_t buff[LEN];
 init_buff(buff);
 if(FifoWritePend(&fifo, buff, LEN, 10)) {
   // Success or partial
 }
 else {
   // Full
 }
 */
len_t FifoWritePend(struct fifo_t *ptr, const void *buff_ptr, len_t length,
                    tick_t ticks_to_wait) {
  len_t val = FifoWrite(ptr, buff_ptr, length);
  if (val == 0) {
    FifoPendWrite(ptr, length, ticks_to_wait);
  }
  return val;
}

/** Pend on character FIFO waiting to read.

 Can be called only by tasks.

 The task will not run until the character FIFO has least length
 used bytes or the timeout expires.

 @param length Maximum number of characters to be written to the character FIFO.
 @param ticks_to_wait Number of ticks the task will wait for the character FIFO
 (timeout). Passing MAX_DELAY the task will not wakeup by timeout.

 Pend on character FIFO waiting to read without timeout:

 FifoPendRead(&fifo, LEN, MAX_DELAY);

 Pend on character FIFO waiting to read with timeout of 10 ticks:

 FifoPendRead(&fifo, LEN, 10);
 */
void FifoPendRead(struct fifo_t *ptr, len_t length, tick_t ticks_to_wait) {
  if (ticks_to_wait != 0U) {
    SchedulerLock();
    INTERRUPTS_DISABLE();
    if (ptr->used < length) {
      struct task_t *task_ptr = GetCurrentTask();
      task_ptr->node_event.value = (tick_t)length; /* length waiting for. */
      OSEventPrePendTask(&ptr->event.list_read, task_ptr);
      INTERRUPTS_ENABLE();
      OSEventPendTask(&ptr->event.list_read, task_ptr, ticks_to_wait);
    } else {
      INTERRUPTS_ENABLE();
    }
    SchedulerUnlock();
  }
}

/** Pend on character FIFO waiting to write.

 Can be called only by tasks.

 The task will not run until the character FIFO has at least length
 free bytes or the timeout expires.

 @param length Maximum number of characters to be written to the character FIFO.
 @param ticks_to_wait Number of ticks the task will wait for the character FIFO
 (timeout). Passing MAX_DELAY the task will not wakeup by timeout.

 Pend on character FIFO waiting to write:

 FifoPendWrite(&fifo,, LEN, MAX_DELAY);

 Pend on character FIFO waiting to write:

 FifoPendWrite(&fifo,, LEN, 10);
 */
void FifoPendWrite(struct fifo_t *ptr, len_t length, tick_t ticks_to_wait) {
  if (ticks_to_wait != 0U) {
    SchedulerLock();
    INTERRUPTS_DISABLE();
    if (ptr->free < length) {
      struct task_t *task_ptr = GetCurrentTask();
      task_ptr->node_event.value = (tick_t)length; /* length waiting for. */
      OSEventPrePendTask(&ptr->event.list_write, task_ptr);
      INTERRUPTS_ENABLE();
      OSEventPendTask(&ptr->event.list_write, task_ptr, ticks_to_wait);
    } else {
      INTERRUPTS_ENABLE();
    }
    SchedulerUnlock();
  }
}

/** Get number of used items on a character FIFO.

 A character FIFO can be read if there is one or more used items.

 @return Number of used items.

 Get number of used items on a character FIFO:

 if(FifoUsed(&fifo) > 0) {
   // Read
 }
 */
len_t FifoUsed(const struct fifo_t *ptr) {
  len_t val;
  CRITICAL_VAL();
  CRITICAL_ENTER();
  { val = ptr->used; }
  CRITICAL_EXIT();
  return val;
}

/** Get number of free items on a character FIFO.

 A character FIFO can be written if there is one or more free items.

 @return Number of free items.

 Get number of free items on a character FIFO:

 if(FifoFree(&fifo) > 0) {
   // Write
 }
 */
len_t FifoFree(const struct fifo_t *ptr) {
  len_t val;
  CRITICAL_VAL();
  CRITICAL_ENTER();
  { val = ptr->free; }
  CRITICAL_EXIT();
  return val;
}

/** Get character FIFO length.

 The character FIFO length is the total number of characters it can hold.

 @return Character FIFO length.

 Get length of a character FIFO:

 len_t alocated_bytes = FifoLength(&fifo);
 */
len_t FifoLength(const struct fifo_t *ptr) {
  /* This value is constant after initialization. No need for locks. */
  return ptr->length;
}

/** Test if a FIFO is empty.

 @return 1 if empty (no items), 0 otherwise.

 if(FifoEmpty(que)) {
   // Do stuff
 }
 */
bool_t FifoEmpty(const struct fifo_t *ptr) { return FifoUsed(ptr) == 0; }

/** Test if a FIFO is full.

 @return 1 if full (no space), 0 otherwise.

 if(FifoFull(que)) {
   // Do stuff
 }
 */
bool_t FifoFull(const struct fifo_t *ptr) { return FifoFree(ptr) == 0; }
