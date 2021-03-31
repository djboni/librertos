/*
 LibreRTOS - Portable single-stack Real Time Operating System.

 Semaphore. Binary and counter semaphores.

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

/** Initialize semaphore.

 @param count Initial count value.
 @param max Max count value.

 Taken binary semaphore:

 struct semaphore_t sem;
 SemaphoreInit(&sem, 0, 1);

 Counting semaphore with maximum count 3:

 struct semaphore_t sem;
 SemaphoreInit(&sem, 3, 3);
 */
void SemaphoreInit(struct semaphore_t *ptr, len_t count, len_t max) {
  ptr->count = count;
  ptr->max = max;
  OSEventRInit(&ptr->event);
}

/** Take semaphore.

 Decrement the semaphore count. Succeed only if the semaphore has been given
 (count is greater than zero).

 @return 1 if success, 0 otherwise.

 Take a semaphore:

 if(SemaphoreTake(&sem)) {
   // Success
 }
 else {
   // Taken
 }
 */
bool_t SemaphoreTake(struct semaphore_t *ptr) {
  bool_t val;
  CRITICAL_VAL();

  CRITICAL_ENTER();
  {
    val = (ptr->count > 0);
    if (val != 0) {
      --ptr->count;
    }
  }
  CRITICAL_EXIT();

  return val;
}

/** Give semaphore.

 Increment the semaphore count. Succeed only if the semaphore has not reached
 its maximum count value.

 @return 1 if success, 0 otherwise.

 Give a semaphore:

 if(SemaphoreGive(&sem)) {
   // Success
 }
 else {
   // Given
 }
 */
bool_t SemaphoreGive(struct semaphore_t *ptr) {
  bool_t val;

  CRITICAL_VAL();

  CRITICAL_ENTER();
  {
    val = (ptr->count < ptr->max);
    if (val != 0) {
      ++ptr->count;

      SchedulerLock();

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

/** Take and, if fails, pend on semaphore.

 Can be called only by tasks.

 The task will not run until the semaphore is given or the timeout expires.

 @param ticks_to_wait Number of ticks the task will wait for the semaphore
 (timeout). Passing MAX_DELAY the task will not wakeup by timeout.
 @return 1 if success, 0 otherwise.

 Take or pend on semaphore without timeout (pend forever):

 if(SemaphoreTakePend(&sem, MAX_DELAY)) {
   // Success
 }
 else {
   // Taken
 }

 Take or pend on semaphore with timeout of 10 ticks:

 if(SemaphoreTakePend(&sem, 10)) {
   // Success
 }
 else {
   // Given
 }
 */
bool_t SemaphoreTakePend(struct semaphore_t *ptr, tick_t ticks_to_wait) {
  bool_t val = SemaphoreTake(ptr);
  if (val == 0) {
    SemaphorePend(ptr, ticks_to_wait);
  }
  return val;
}

/** Pend on semaphore.

 Can be called only by tasks.

 The task will not run until the semaphore is given or the timeout expires.

 @param ticks_to_wait Number of ticks the task will wait for the semaphore
 (timeout). Passing MAX_DELAY the task will not wakeup by timeout.

 Pend on semaphore without (pend forever):

 SemaphorePend(&sem, MAX_DELAY);

 Pend on semaphore with timeout of 10 ticks:

 SemaphorePend(&sem, 10);
 */
void SemaphorePend(struct semaphore_t *ptr, tick_t ticks_to_wait) {
  if (ticks_to_wait != 0U) {
    struct task_t *task_ptr = GetCurrentTask();

    SchedulerLock();
    INTERRUPTS_DISABLE();
    if (ptr->count == 0U) {
      OSEventPrePendTask(&ptr->event.list_read, task_ptr);
      INTERRUPTS_ENABLE();
      OSEventPendTask(&ptr->event.list_read, task_ptr, ticks_to_wait);
    } else {
      INTERRUPTS_ENABLE();
    }
    SchedulerUnlock();
  }
}

/** Get semaphore count value.

 @return Semaphore count value.

 Get semaphore count value:

 if(SemaphoreGetCount(&sem) == 0) {
   // Taken
 }
 else {
   // Given
 }
 */
len_t SemaphoreGetCount(const struct semaphore_t *ptr) {
  len_t val;
  CRITICAL_VAL();
  CRITICAL_ENTER();
  { val = ptr->count; }
  CRITICAL_EXIT();
  return val;
}

/** Get semaphore maximum count value.

 @return Semaphore maximum count value.

 Get semaphore maximum count value:

 if(SemaphoreGetMax(&sem) == 1) {
   // Binary semaphore
 }
 else {
   // Counting semaphore
 }
 */
len_t SemaphoreGetMax(const struct semaphore_t *ptr) {
  /* This value is constant after initialization. No need for locks. */
  return ptr->max;
}
