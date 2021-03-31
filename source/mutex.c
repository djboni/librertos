/*
 LibreRTOS - Portable single-stack Real Time Operating System.

 Mutex. Recursive mutex. No priority inheritance mechanism.

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

#define MUTEX_NOT_OWNED ((struct task_t *)NULL)

/** Initialize mutex.

 Initialize mutex:

 struct mutex_t mtx;
 MutexInit(&mtx);
 */
void MutexInit(struct mutex_t *ptr) {
  ptr->count = 0;
  ptr->mutex_owner_ptr = MUTEX_NOT_OWNED;
  OSEventRInit(&ptr->event);
}

/** Lock mutex.

 Can be called only by tasks.

 The task that owns the mutex can lock it multiple times. To completely
 unlock the mutex the task must unlock it the same number of times it was
 locked.

 @return 1 if success, 0 otherwise.

 Lock a mutex:

 if(MutexLock(&mtx)) {
   // Success
   MutexUnlock();
 }
 else {
   // Locked
 }
 */
bool_t MutexLock(struct mutex_t *ptr) {
  bool_t val;

  INTERRUPTS_DISABLE();
  {
    struct task_t *current_task_ptr = GetCurrentTask();
    val = ptr->count == 0 || ptr->mutex_owner_ptr == current_task_ptr;
    if (val != 0) {
      ++ptr->count;
      ptr->mutex_owner_ptr = current_task_ptr;
    }
  }
  INTERRUPTS_ENABLE();

  return val;
}

/** Unlock mutex.

 Can be called by tasks and by interrupts.

 The task that owns the mutex can lock it multiple times. To completely
 unlock the mutex the task must unlock it the same number of times it was
 locked.

 @return 1 if success, 0 otherwise.

 Unlock a mutex:

 if(MutexLock(&mtx)) {
   // Success
   MutexUnlock();
 }
 else {
   // Locked
 }
 */
bool_t MutexUnlock(struct mutex_t *ptr) {
  bool_t val;
  CRITICAL_VAL();

  CRITICAL_ENTER();
  {
    val = ptr->count > 0;

    if (val != 0) {
      --ptr->count;

      SchedulerLock();

      if (ptr->count == 0) {
        ptr->mutex_owner_ptr = NULL;

        if (ptr->event.list_read.length != 0) {
          /* Unblock tasks waiting to read from this event. */
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

/** Lock and, if fails, pend on mutex.

 Can be called only by tasks.

 The task will not run until the mutex is unlocked or the timeout expires.

 @param ticks_to_wait Number of ticks the task will wait for the mutex
 (timeout). Passing MAX_DELAY the task will not wakeup by timeout.
 @return 1 if success, 0 otherwise.

 Lock or pend on mutex without timeout:

 if(MutexLockPend(&mtx, MAX_DELAY)) {
   // Success
 }
 else {
   // Locked
 }

 Lock or pend on mutex with timeout of 10 ticks:

 if(MutexLockPend(&mtx, 10)) {
   // Success
 }
 else {
   // Locked
 }
 */
bool_t MutexLockPend(struct mutex_t *ptr, tick_t ticks_to_wait) {
  bool_t val = MutexLock(ptr);
  if (val == 0) {
    MutexPend(ptr, ticks_to_wait);
  }
  return val;
}

/** Pend on mutex.

 Can be called only by tasks.

 The task will not run until the mutex is unlocked or the timeout expires.

 @param ticks_to_wait Number of ticks the task will wait for the mutex
 (timeout). Passing MAX_DELAY the task will not wakeup by timeout.

 Lock or pend on mutex without timeout:

 MutexPend(&mtx, MAX_DELAY);

 Lock or pend on mutex with timeout of 10 ticks:

 MutexPend(&mtx, 10);
 */
void MutexPend(struct mutex_t *ptr, tick_t ticks_to_wait) {
  if (ticks_to_wait != 0U) {
    struct task_t *task_ptr = GetCurrentTask();

    SchedulerLock();
    INTERRUPTS_DISABLE();
    if (ptr->count != 0 && ptr->mutex_owner_ptr != task_ptr) {
      OSEventPrePendTask(&ptr->event.list_read, task_ptr);
      INTERRUPTS_ENABLE();
      OSEventPendTask(&ptr->event.list_read, task_ptr, ticks_to_wait);
    } else {
      INTERRUPTS_ENABLE();
    }
    SchedulerUnlock();
  }
}

/** Get mutex count value.

 @return Number of times the owner of the mutex recursively locked it.

 Get mutex count value:

 if(MutexGetCount(&mtx) == 0) {
   // Unlocked
 }
 else {
   // Locked
 }
 */
len_t MutexGetCount(const struct mutex_t *ptr) {
  len_t val;
  CRITICAL_VAL();
  CRITICAL_ENTER();
  { val = ptr->count; }
  CRITICAL_EXIT();
  return val;
}

/** Get mutex owner.

 @return Pointer to the task that locked the mutex.

 Get mutex owner:

 if(Mutex_getOwner(&mtx) == NULL) {
   // Unlocked
 }
 else {
   // Locked
 }
 */
struct task_t *MutexGetOwner(const struct mutex_t *ptr) {
  struct task_t *task_ptr;
  CRITICAL_VAL();
  CRITICAL_ENTER();
  { task_ptr = ptr->mutex_owner_ptr; }
  CRITICAL_EXIT();
  return task_ptr;
}
