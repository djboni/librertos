/*
 LibreRTOS - Portable single-stack Real Time Operating System.

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
#include <stddef.h>

#ifndef LIBRERTOS_GUARD_U32
#define LIBRERTOS_GUARD_U32 0xFA57C0DEUL
#endif

#if (LIBRERTOS_PREEMPT_LIMIT < 0)
#error "LIBRERTOS_PREEMPT_LIMIT < 0! Should be >= 0."
#endif

#if (LIBRERTOS_PREEMPT_LIMIT >= LIBRERTOS_MAX_PRIORITY)
#error                                                                         \
    "LIBRERTOS_PREEMPT_LIMIT >= LIBRERTOS_MAX_PRIORITY! Makes the kernel cooperative! Should be < LIBRERTOS_MAX_PRIORITY."
#endif

/**
 Initialize OS.

 Must be called before any other OS function.

 LibrertosInit();
 // Peripherals, data sctructures, and tasks init
 LibrertosStart();
 for(;;) {
   LibrertosScheduler();
 }
 */
void LibrertosInit(void) {
  uint16_t i;

  OS_State.scehduler_lock = 1;
  OS_State.scheduler_unlock_todo = 0;
  OS_State.current_tcb_ptr = NULL;

#if (LIBRERTOS_PREEMPTION != 0)
  { OS_State.higher_ready_task = 0; }
#endif

  for (i = 0; i < LIBRERTOS_MAX_PRIORITY; ++i) {
    OS_State.task_ptr[i] = NULL;
  }

  OS_State.tick = 0U;
  OS_State.delayed_ticks = 0U;

  OS_State.blocked_task_list_not_overflowed_ptr = &OS_State.blocked_task_list_1;
  OS_State.blocked_task_list_overflowed_ptr = &OS_State.blocked_task_list_2;

  OSListHeadInit(&OS_State.pending_ready_task_list);
  OSListHeadInit(&OS_State.blocked_task_list_1);
  OSListHeadInit(&OS_State.blocked_task_list_2);

#if (LIBRERTOS_SOFTWARETIMERS != 0)
  {
    OS_State.task_timer_last_run = 0;
    /* OS_State.TaskTimerTCB is initialized when calling OS_timerTaskCreate().
     */
    OS_State.timer_index_ptr = (struct task_list_node_t *)&OS_State.timer_list;
    OSListHeadInit(&OS_State.timer_list);
    OSListHeadInit(&OS_State.timer_unordered_list);
  }
#endif

#if (LIBRERTOS_STATISTICS != 0)
  {
    OS_State.total_run_time = 0;
    OS_State.no_task_run_time = 0;
  }
#endif

#if (LIBRERTOS_STATE_GUARDS != 0)
  {
    OS_State.guard_start = LIBRERTOS_GUARD_U32;
    OS_State.guard_end = LIBRERTOS_GUARD_U32;
  }
#endif
}

/** Start OS.

 Must be called once before the scheduler.

 LibrertosInit();
 // Peripherals, data sctructures, and tasks init
 LibrertosStart();
 for(;;) {
   LibrertosScheduler();
 }
 */
void LibrertosStart(void) {
  INTERRUPTS_ENABLE();
  SchedulerUnlock();
}

/** Invert blocked tasks lists.

 Called by unblock timedout tasks function.
 */
void OSTickInvertBlockedTasksLists(void) {
  struct task_head_list_t *temp_ptr =
      OS_State.blocked_task_list_not_overflowed_ptr;
  OS_State.blocked_task_list_not_overflowed_ptr =
      OS_State.blocked_task_list_overflowed_ptr;
  OS_State.blocked_task_list_overflowed_ptr = temp_ptr;
}

/** Increment OS tick.

 Called by the tick interrupt (defined by the user).

 void Timer0OverflowInterrupt(void) {
   LibrertosTick();
 }
 */
void LibrertosTick(void) {
  SchedulerLock();

  ++OS_State.delayed_ticks;

  /* Scheduler unlock has work todo. */
  OS_State.scheduler_unlock_todo = 1;

  SchedulerUnlock();
}

/** Schedule a task.

 Called by scheduler.
 */
void OSScheduleTask(struct task_t *const task_ptr) {
  struct task_t *current_task_ptr;

  /* Get task function and parameter. */
  task_function_t task_function = task_ptr->function;
  task_parameter_t task_parameter = task_ptr->parameter;

  /* Save and set current TCB. */
  INTERRUPTS_DISABLE();

  /* Inside critical section. We can read CurrentTCB directly. */
  current_task_ptr = OS_State.current_tcb_ptr;

  OS_State.current_tcb_ptr = task_ptr;

#if (LIBRERTOS_STATISTICS != 0)
  {
    stattime_t now = LibrertosSystemRunTime();
    OS_State.no_task_run_time += now - OS_State.total_run_time;
    OS_State.total_run_time = now;
    ++task_ptr->task_num_sched;
  }
#endif

  INTERRUPTS_ENABLE();

  SchedulerUnlock();

  /* Run task. */
  task_function(task_parameter);

  SchedulerLock();

  /* Restore last TCB. */
  INTERRUPTS_DISABLE();

  OS_State.current_tcb_ptr = current_task_ptr;

#if (LIBRERTOS_STATISTICS != 0)
  {
    stattime_t now = LibrertosSystemRunTime();
    current_task_ptr->task_run_time += now - OS_State.total_run_time;
    OS_State.total_run_time = now;
  }
#endif

  INTERRUPTS_ENABLE();
}

/**
 Schedule a task.

 Called in the main loop.

 LibrertosInit();
 // Peripherals, data sctructures, and tasks init
 LibrertosStart();
 for(;;) {
   LibrertosScheduler();
 }
 */
void LibrertosScheduler(void) {
  SchedulerLock();
  OSSchedulerReal();
  SchedulerUnlock();
}

/** Scheduler algorithm a task. Called by scheduler functions and by scheduler
 unlock function. */
void OSSchedulerReal(void) {
#if (LIBRERTOS_STATISTICS != 0)
  {
    stattime_t now;
    /* Scheduler locked. We can read current_tcb_ptr directly. */
    struct task_t *current_task_ptr = OS_State.current_tcb_ptr;
    INTERRUPTS_DISABLE();
    now = LibrertosSystemRunTime();
    if (current_task_ptr != NULL) {
      current_task_ptr->task_run_time += now - OS_State.total_run_time;
    } else {
      OS_State.no_task_run_time += now - OS_State.total_run_time;
    }
    OS_State.total_run_time = now;
    INTERRUPTS_ENABLE();
  }
#endif

  for (;;) {
    /* Schedule higher priority task. */

    /* Save current TCB. */
    struct task_t *task_ptr = NULL;

    {
      /* Scheduler locked. We can read CurrentTCB directly. */
      priority_t current_task_priority =
          (OS_State.current_tcb_ptr == NULL
               ? LIBRERTOS_NO_TASK_RUNNING
               : OS_State.current_tcb_ptr->priority);
      priority_t priority;

      for (priority = LIBRERTOS_MAX_PRIORITY - 1;
           priority > current_task_priority; --priority) {
        /* Atomically test TaskState. */
        INTERRUPTS_DISABLE();
        if (OS_State.task_ptr[priority] != NULL) {
          /* Higher priority task ready. */

#if (LIBRERTOS_PREEMPTION != 0 && LIBRERTOS_PREEMPT_LIMIT > 0)
          {
            /* Schedule only if preemption limit allows it. */
            if (priority >= LIBRERTOS_PREEMPT_LIMIT ||
                current_task_priority == LIBRERTOS_NO_TASK_RUNNING) {
              task_ptr = OS_State.task_ptr[priority];
            }
          }
#else  /* LIBRERTOS_PREEMPTION && LIBRERTOS_PREEMPT_LIMIT */
          { task_ptr = OS_State.task_ptr[priority]; }
#endif /* LIBRERTOS_PREEMPTION && LIBRERTOS_PREEMPT_LIMIT */

          INTERRUPTS_ENABLE();
          break;
        }
        INTERRUPTS_ENABLE();
      }
    }

    if (task_ptr != NULL) {
      /* Higher priority task ready. */
      OSScheduleTask(task_ptr);
    } else {
      /* No higher priority task ready. */
      break;
    }

    /* Loop to check if another higher priority task is ready. */

  } /* for(;;) */

#if (LIBRERTOS_STATISTICS != 0)
  {
    stattime_t now;
    INTERRUPTS_DISABLE();
    now = LibrertosSystemRunTime();
    OS_State.no_task_run_time += now - OS_State.total_run_time;
    OS_State.total_run_time = now;
    INTERRUPTS_ENABLE();
  }
#endif
}

/** Lock scheduler (recursive lock).

 Current task cannot be preempted if scheduler is locked.
 */
void SchedulerLock(void) { ++OS_State.scehduler_lock; }

/** Unblock tasks that have timedout (process OS ticks).

 Called by scheduler unlock function.
 */
void OSTickUnblockTimedoutTasks(void) {
  /* Unblock tasks that have timed-out. */

  if (OS_State.tick == 0) {
    OSTickInvertBlockedTasksLists();
  }

  while (OS_State.blocked_task_list_not_overflowed_ptr->length != 0 &&
         OS_State.blocked_task_list_not_overflowed_ptr->head_ptr->value ==
             OS_State.tick) {
    struct task_t *task_ptr =
        (struct task_t *)
            OS_State.blocked_task_list_not_overflowed_ptr->head_ptr->owner_ptr;

    /* Remove from blocked list. */
    OSListRemove(&task_ptr->node_delay);

    INTERRUPTS_DISABLE();

    task_ptr->state = TASKSTATE_READY;

    /* Remove from event list. */
    if (task_ptr->node_event.list_ptr != NULL) {
      OSListRemove(&task_ptr->node_event);
    }

    OS_State.task_ptr[task_ptr->priority] = task_ptr;

#if (LIBRERTOS_PREEMPTION != 0)
    {
#if (LIBRERTOS_PREEMPT_LIMIT > 0)
      if (task_ptr->priority >= LIBRERTOS_PREEMPT_LIMIT) {
#endif

        /* Inside critical section. We can read CurrentTCB directly. */
        if (OS_State.current_tcb_ptr == NULL ||
            task_ptr->priority > OS_State.current_tcb_ptr->priority) {
          OS_State.higher_ready_task = 1;
        }

#if (LIBRERTOS_PREEMPT_LIMIT > 0)
      }
#endif
    }
#endif /* LIBRERTOS_PREEMPTION */

    INTERRUPTS_ENABLE();
  }
}

/** Unblock pending ready tasks.

 Called by scheduler unlock function.
 */
void OSTickUnblockPendingReadyTasks(void) {
  INTERRUPTS_DISABLE();
  while (OS_State.pending_ready_task_list.length != 0) {
    struct task_t *task_ptr =
        (struct task_t *)OS_State.pending_ready_task_list.head_ptr->owner_ptr;

    /* Remove from pending ready list. */
    OSListRemove(&task_ptr->node_event);

#if (LIBRERTOS_PREEMPTION != 0)
    {
#if (LIBRERTOS_PREEMPT_LIMIT > 0)
      if (task_ptr->priority >= LIBRERTOS_PREEMPT_LIMIT) {
#endif

        /* Inside critical section. We can read CurrentTCB directly. */
        if (OS_State.current_tcb_ptr == NULL ||
            task_ptr->priority > OS_State.current_tcb_ptr->priority) {
          OS_State.higher_ready_task = 1;
        }

#if (LIBRERTOS_PREEMPT_LIMIT > 0)
      }
#endif
    }
#endif /* LIBRERTOS_PREEMPTION */

    INTERRUPTS_ENABLE();

    /* Remove from blocked list. */
    if (task_ptr->node_delay.list_ptr != NULL) {
      OSListRemove(&task_ptr->node_delay);
    }

    task_ptr->state = TASKSTATE_READY;

    INTERRUPTS_DISABLE();

    OS_State.task_ptr[task_ptr->priority] = task_ptr;
  }
  INTERRUPTS_ENABLE();
}

/** Unlock scheduler (recursive lock).

 Current task may be preempted if scheduler is unlocked.
 */
void SchedulerUnlock(void) {
  if (OS_State.scheduler_unlock_todo != 0 && OS_State.scehduler_lock == 1) {
    CRITICAL_VAL();
    CRITICAL_ENTER();

    for (;;) {
      OS_State.scheduler_unlock_todo = 0;

      while (OS_State.delayed_ticks != 0) {
        --OS_State.delayed_ticks;
        ++OS_State.tick;

        INTERRUPTS_ENABLE();
        OSTickUnblockTimedoutTasks();
        INTERRUPTS_DISABLE();
      }

      INTERRUPTS_ENABLE();

      OSTickUnblockPendingReadyTasks();

      INTERRUPTS_DISABLE();

#if (LIBRERTOS_PREEMPTION != 0)
      {
        if (OS_State.higher_ready_task != 0) {
          OS_State.higher_ready_task = 0;
          INTERRUPTS_ENABLE();
          OSSchedulerReal();
          INTERRUPTS_DISABLE();
        }
      }
#endif /* LIBRERTOS_PREEMPTION */

      if (OS_State.scheduler_unlock_todo == 0) {
        /* Scheduler unlock has no work todo. */
        break;
      }
    }

    --OS_State.scehduler_lock;

    CRITICAL_EXIT();
  } else {
    --OS_State.scehduler_lock;
  }
}

/** Create task.

 @param priority Task priority from 0 (lower) to LIBRERTOS_MAX_PRIORITY - 1
 (higher).
 @param function Task function with prototype void function(void *).
 @param parameter Task parameter with type void *.

 task_t task_idle;
 task_t task_blink;
 task_t task_serial;
 LibrertosTaskCreate(&task_idle,   0, &TaskIdle,   NULL);
 LibrertosTaskCreate(&task_blink,  1, &TaskBlink,  NULL);
 LibrertosTaskCreate(&task_serial, 2, &TaskSerial, NULL);
 */
void LibrertosTaskCreate(struct task_t *ptr, priority_t priority,
                         task_function_t function, task_parameter_t parameter) {
  ptr->state = TASKSTATE_READY;
  ptr->function = function;
  ptr->parameter = parameter;
  ptr->priority = priority;

  OSListNodeInit(&ptr->node_delay, ptr);
  OSListNodeInit(&ptr->node_event, ptr);

#if (LIBRERTOS_STATISTICS != 0)
  {
    ptr->task_run_time = 0;
    ptr->task_num_sched = 0;
  }
#endif

  OS_State.task_ptr[priority] = ptr;
}

/** Return current task priority. */
struct task_t *GetCurrentTask(void) {
  struct task_t *task_ptr;
  CRITICAL_VAL();
  CRITICAL_ENTER();
  { task_ptr = OS_State.current_tcb_ptr; }
  CRITICAL_EXIT();
  return task_ptr;
}

/** Delay task.

 Can only be called by tasks.

 @param ticks_to_delay Number of system ticks to wait before resuming the task.

 void TaskBlink(void *param_ptr) {
   (void*)param_ptr;
   LEDToggle();
   TaskDelay(50);
 }
*/
void TaskDelay(tick_t ticks_to_delay) {
  /* Insert task in the blocked tasks list. */

  SchedulerLock();
  if (ticks_to_delay != 0) {
    tick_t tick_now = (tick_t)(OS_State.tick + OS_State.delayed_ticks);
    tick_t tick_to_wakeup = (tick_t)(tick_now + ticks_to_delay);

    struct task_t *task_ptr = GetCurrentTask();
    struct task_list_node_t *task_blocked_node_ptr = &task_ptr->node_delay;
    struct task_head_list_t *blocked_task_tist_ptr;

    if (tick_to_wakeup > tick_now) {
      /* Not overflowed. */
      blocked_task_tist_ptr = OS_State.blocked_task_list_not_overflowed_ptr;
    } else {
      /* Overflowed. */
      blocked_task_tist_ptr = OS_State.blocked_task_list_overflowed_ptr;
    }

    INTERRUPTS_DISABLE();
    task_ptr->state = TASKSTATE_BLOCKED;
    OS_State.task_ptr[task_ptr->priority] = NULL;
    INTERRUPTS_ENABLE();

    /* Insert task on list. */
    OSListInsert(blocked_task_tist_ptr, task_blocked_node_ptr, tick_to_wakeup);
  }
  SchedulerUnlock();
}

/** Resume task. */
void TaskResume(struct task_t *ptr) {
  struct task_list_node_t *node_ptr = &ptr->node_event;
  CRITICAL_VAL();

  SchedulerLock();
  {
    CRITICAL_ENTER();
    {
      /* Remove from event list. */
      if (node_ptr->list_ptr != NULL) {
        OSListRemove(&ptr->node_event);
      }

      /* Add to pending ready tasks list. */
      OSListInsertAfter(&OS_State.pending_ready_task_list,
                        OS_State.pending_ready_task_list.head_ptr, node_ptr);

      /* Scheduler unlock has work todo. */
      OS_State.scheduler_unlock_todo = 1;
    }
    CRITICAL_EXIT();
  }
  SchedulerUnlock();
}

/** Get current OS tick.

 @return Number of system ticks since start (may overflow).
*/
tick_t GetTickCount(void) {
  tick_t tick_now;
  SchedulerLock();
  tick_now = OS_State.tick;
  SchedulerUnlock();
  return tick_now;
}

#if (LIBRERTOS_STATE_GUARDS != 0)

/** Check OS_state guards.

 @return 1 if OS_State guards are fine, 0 otherwise. */
bool_t LibrertosStateCheck(void) {
  return (OS_State.guard_start == LIBRERTOS_GUARD_U32 &&
          OS_State.guard_end == LIBRERTOS_GUARD_U32);
}

#endif /* LIBRERTOS_STATE_GUARDS */

#if (LIBRERTOS_STATISTICS != 0)

/** Get total system run time.

 @return Total system run time.
 */
stattime_t LibrertosTotalRunTime(void) {
  stattime_t val;
  CRITICAL_VAL();
  CRITICAL_ENTER();
  val = OS_State.total_run_time;
  CRITICAL_EXIT();
  return val;
}

/** Get no task run time (time scheduling tasks).

 @return No task run time.
 */
stattime_t LibrertosNoTaskRunTime(void) {
  stattime_t val;
  CRITICAL_VAL();
  CRITICAL_ENTER();
  val = OS_State.no_task_run_time;
  CRITICAL_EXIT();
  return val;
}

/** Get the run time of a given task.

 @param task_ptr Task pointer.
 @return Task run time.
 */
stattime_t LibrertosTaskRunTime(const struct task_t *ptr) {
  stattime_t val;
  CRITICAL_VAL();
  CRITICAL_ENTER();
  val = ptr->task_run_time;
  CRITICAL_EXIT();
  return val;
}

/** Get the number of times a given task was scheduled.

 @param task_ptr Task pointer.
 @return Number of schedules of the task.
 */
stattime_t LibrertosTaskNumSchedules(const struct task_t *ptr) {
  stattime_t val;
  CRITICAL_VAL();
  CRITICAL_ENTER();
  val = ptr->task_num_sched;
  CRITICAL_EXIT();
  return val;
}

#endif /* LIBRERTOS_STATISTICS */
