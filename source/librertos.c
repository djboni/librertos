/*
 LibreRTOS - Portable single-stack Real Time Operating System.

 Scheduler.
 Linked list.
 Pend on events.

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

static void OSTickInvertBlockedTasksLists(void);
static void OSTickUnblockTimedoutTasks(void);
static void OSTickUnblockPendingReadyTasks(void);
static void OSSchedulerReal(void);
static void OSScheduleTask(struct task_t *const task_ptr);

/** Initialize OS. Must be called before any other OS function. */
void LibrertosInit(void) {
  uint16_t i;

  OS_State.scehduler_lock = 1;
  OS_State.scheduler_unlock_todo = 0;
  OS_State.current_tcb_ptr = NULL;

#if (LIBRERTOS_PREEMPTION != 0)
  { OS_State.higher_ready_task = 0; }
#endif

  OSListHeadInit(&OS_State.pending_ready_task_list);

  OS_State.tick = 0U;
  OS_State.delayed_ticks = 0U;
  OS_State.blocked_task_list_not_overflowed_ptr = &OS_State.blocked_task_list_1;
  OS_State.blocked_task_list_overflowed_ptr = &OS_State.blocked_task_list_2;
  OSListHeadInit(&OS_State.blocked_task_list_1);
  OSListHeadInit(&OS_State.blocked_task_list_2);

  for (i = 0; i < LIBRERTOS_MAX_PRIORITY; ++i) {
    OS_State.task_ptr[i] = NULL;
  }

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

/** Start OS. Must be called once before the scheduler. */
void LibrertosStart(void) {
  INTERRUPTS_ENABLE();
  SchedulerUnlock();
}

/* Invert blocked tasks lists. Called by unblock timedout tasks function. */
static void OSTickInvertBlockedTasksLists(void) {
  struct task_head_list_t *temp_ptr =
      OS_State.blocked_task_list_not_overflowed_ptr;
  OS_State.blocked_task_list_not_overflowed_ptr =
      OS_State.blocked_task_list_overflowed_ptr;
  OS_State.blocked_task_list_overflowed_ptr = temp_ptr;
}

/** Increment OS tick. Called by the tick interrupt (defined by the
 user). */
void LibrertosTick(void) {
  SchedulerLock();

  ++OS_State.delayed_ticks;

  /* Scheduler unlock has work todo. */
  OS_State.scheduler_unlock_todo = 1;

  SchedulerUnlock();
}

/** Schedule a task. Called by scheduler. */
static void OSScheduleTask(struct task_t *const task_ptr) {
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

/** Schedule a task. Called in the main loop. */
void LibrertosScheduler(void) {
  SchedulerLock();
  OSSchedulerReal();
  SchedulerUnlock();
}

/** Scheduler algorithm a task. Called by scheduler functions and by scheduler
 unlock function. */
static void OSSchedulerReal(void) {
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

/** Lock scheduler (recursive lock). Current task cannot be preempted if
 scheduler is locked. */
void SchedulerLock(void) { ++OS_State.scehduler_lock; }

/* Unblock tasks that have timedout (process OS ticks). Called by scheduler
 unlock function. */
static void OSTickUnblockTimedoutTasks(void) {
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

/* Unblock pending ready tasks. Called by scheduler unlock function. */
static void OSTickUnblockPendingReadyTasks(void) {
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

/** Unlock scheduler (recursive lock). Current task may be preempted if
 scheduler is unlocked. */
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

/** Create task. */
void LibrertosTaskCreate(struct task_t *ptr, priority_t priority,
                task_function_t function, task_parameter_t parameter) {
  ASSERT(priority < LIBRERTOS_MAX_PRIORITY);
  ASSERT(OS_State.task_ptr[priority] == NULL);

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

/** Delay task. */
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
void TaskResume(struct task_t *task_ptr) {
  struct task_list_node_t *node_ptr = &task_ptr->node_event;
  CRITICAL_VAL();

  SchedulerLock();
  {
    CRITICAL_ENTER();
    {
      /* Remove from event list. */
      if (node_ptr->list_ptr != NULL) {
        OSListRemove(&task_ptr->node_event);
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

/** Get current OS tick. */
tick_t GetTickCount(void) {
  tick_t tick_now;
  SchedulerLock();
  tick_now = OS_State.tick;
  SchedulerUnlock();
  return tick_now;
}

#if (LIBRERTOS_STATE_GUARDS != 0)

/** Return 1 if OS_State guards are fine. 0 otherwise. */
bool_t LibrertosStateCheck(void) {
  return (OS_State.guard_start == LIBRERTOS_GUARD_U32 &&
          OS_State.guard_end == LIBRERTOS_GUARD_U32);
}

#endif /* LIBRERTOS_STATE_GUARDS */

#if (LIBRERTOS_STATISTICS != 0)

stattime_t LibrertosTotalRunTime(void) {
  stattime_t val;
  CRITICAL_VAL();
  CRITICAL_ENTER();
  val = OS_State.total_run_time;
  CRITICAL_EXIT();
  return val;
}

stattime_t LibrertosNoTaskRunTime(void) {
  stattime_t val;
  CRITICAL_VAL();
  CRITICAL_ENTER();
  val = OS_State.no_task_run_time;
  CRITICAL_EXIT();
  return val;
}

stattime_t LibrertosTaskRunTime(const struct task_t *ptr) {
  stattime_t val;
  CRITICAL_VAL();
  CRITICAL_ENTER();
  val = ptr->task_run_time;
  CRITICAL_EXIT();
  return val;
}

stattime_t LibrertosTaskNumSchedules(const struct task_t *ptr) {
  stattime_t val;
  CRITICAL_VAL();
  CRITICAL_ENTER();
  val = ptr->task_num_sched;
  CRITICAL_EXIT();
  return val;
}

#endif /* LIBRERTOS_STATISTICS */

#define LIST_HEAD(x) ((struct task_list_node_t *)x)

/** Initialize list head. */
void OSListHeadInit(struct task_head_list_t *ptr) {
  /* Use the list head as a node. */
  ptr->head_ptr = LIST_HEAD(ptr);
  ptr->tail_ptr = LIST_HEAD(ptr);
  ptr->length = 0;
}

/** Initialize list node. */
void OSListNodeInit(struct task_list_node_t *ptr, void *owner_ptr) {
  ptr->next_ptr = NULL;
  ptr->prev_ptr = NULL;
  ptr->value = 0;
  ptr->list_ptr = NULL;
  ptr->owner_ptr = owner_ptr;
}

/** Insert node into list. Position according to value. */
void OSListInsert(struct task_head_list_t *ptr,
                  struct task_list_node_t *node_ptr, tick_t value) {
  struct task_list_node_t *pos_ptr = ptr->head_ptr;

  while (pos_ptr != LIST_HEAD(ptr)) {
    if (value >= pos_ptr->value) {
      /* Not here. */
      pos_ptr = pos_ptr->next_ptr;
    } else {
      /* Insert here. */
      break;
    }
  }

  node_ptr->value = value;
  node_ptr->list_ptr = ptr;

  node_ptr->next_ptr = pos_ptr;
  node_ptr->prev_ptr = pos_ptr->prev_ptr;

  pos_ptr->prev_ptr->next_ptr = node_ptr;
  pos_ptr->prev_ptr = node_ptr;

  ++ptr->length;
}

/** Insert node into list after pos. */
void OSListInsertAfter(struct task_head_list_t *ptr,
                       struct task_list_node_t *pos_ptr,
                       struct task_list_node_t *node_ptr) {
  node_ptr->list_ptr = ptr;

  node_ptr->next_ptr = pos_ptr->next_ptr;
  node_ptr->prev_ptr = pos_ptr;

  pos_ptr->next_ptr->prev_ptr = node_ptr;
  pos_ptr->next_ptr = node_ptr;

  ++ptr->length;
}

/** Remove node from list. */
void OSListRemove(struct task_list_node_t *ptr) {
  struct task_list_node_t *next_ptr = ptr->next_ptr;
  struct task_list_node_t *prev_ptr = ptr->prev_ptr;

  --ptr->list_ptr->length;

  next_ptr->prev_ptr = prev_ptr;
  prev_ptr->next_ptr = next_ptr;

  ptr->next_ptr = NULL;
  ptr->prev_ptr = NULL;
  ptr->list_ptr = NULL;
}

/** Initialize event (read) struct. */
void OSEventRInit(struct event_r_t *ptr) { OSListHeadInit(&ptr->list_read); }

/** Initialize event (read/write) struct. */
void OSEventRwInit(struct event_rw_t *ptr) {
  OSListHeadInit(&ptr->list_read);
  OSListHeadInit(&ptr->list_write);
}

/** Pend task on an event (part 1). Must be called with interrupts disabled and
 scheduler locked. */
void OSEventPrePendTask(struct task_head_list_t *list_ptr,
                        struct task_t *task_ptr) {
  /* Put task on the head position in the event list. So the task may be
   unblocked by an interrupt. */

  struct task_list_node_t *node_ptr = &task_ptr->node_event;
  OSListInsertAfter(list_ptr, (struct task_list_node_t *)list_ptr, node_ptr);
}

/** Pend task on an event (part 2). Must be called with interrupts enabled and
 scheduler locked. Parameter ticks_to_wait must not be zero. */
void OSEventPendTask(struct task_head_list_t *list_ptr, struct task_t *task_ptr,
                     tick_t ticks_to_wait) {
  struct task_list_node_t *node_ptr = &task_ptr->node_event;
  priority_t priority = task_ptr->priority;

  INTERRUPTS_DISABLE();

  /* Find correct position for the task in the event list. This list may
   be changed by interrupts, so we must do things carefully. */
  {
    struct task_list_node_t *pos_ptr;

    for (;;) {
      pos_ptr = list_ptr->tail_ptr;

      while (pos_ptr != LIST_HEAD(list_ptr)) {
        INTERRUPTS_ENABLE();

        /* For test coverage only. This macro is used as a deterministic
         way to create a concurrent access. */
        LIBRERTOS_TEST_CONCURRENT_ACCESS();

        if (((struct task_t *)pos_ptr->owner_ptr)->priority <= priority) {
          /* Found where to insert. Break while(). */
          INTERRUPTS_DISABLE();
          break;
        }

        INTERRUPTS_DISABLE();
        if (pos_ptr->list_ptr != list_ptr) {
          /* This position was removed from the list. An interrupt
           resumed this task. Break while(). */
          break;
        }

        /* As this task is inserted in the head of the list, if an
         interrupt resumed the task then pos_ptr also must have been
         modified.
         So break the while loop if current task was changed is
         redundant. */

        pos_ptr = pos_ptr->prev_ptr;
      }

      if (pos_ptr != LIST_HEAD(list_ptr) && pos_ptr->list_ptr != list_ptr &&
          node_ptr->list_ptr == list_ptr) {
        /* This pos_ptr was removed from the list and node_ptr was not
         removed. Must restart to find where to insert node.
         Continue for(;;). */
        continue;
      } else {
        /* Found where to insert. Insert after pos_ptr.
         OR
         Item node was removed from the list (interrupt resumed the
         task). Nothing to insert.
         Break for(;;). */
        break;
      }
    }

    if (node_ptr->list_ptr == list_ptr) {
      /* If an interrupt didn't resume the task. */

      /* Don't need to remove and insert if the task is already in its
       position. */

      if (node_ptr != pos_ptr) {
        /* Now insert in the right position. */
        OSListRemove(node_ptr);
        OSListInsertAfter(list_ptr, pos_ptr, node_ptr);
      }

      /* Suspend or block task. */
      /* Ticks enabled. Suspend if ticks to wait is maximum delay, block with
          timeout otherwise. */
      if (ticks_to_wait == MAX_DELAY) {
        task_ptr->state = TASKSTATE_SUSPENDED;
        OS_State.task_ptr[task_ptr->priority] = NULL;
        INTERRUPTS_ENABLE();
      } else {
        INTERRUPTS_ENABLE();
        TaskDelay(ticks_to_wait);
      }
    } else {
      INTERRUPTS_ENABLE();
    }
  }
}

/** Unblock task in an event list. Must be called with scheduler locked and in a
 critical section. */
void OSEventUnblockTasks(struct task_head_list_t *list_ptr) {
  if (list_ptr->length != 0) {
    struct task_list_node_t *node_ptr = list_ptr->tail_ptr;

    /* Remove from event list. */
    OSListRemove(node_ptr);

    /* Insert in the pending ready tasks . */
    OSListInsertAfter(&OS_State.pending_ready_task_list,
                      OS_State.pending_ready_task_list.head_ptr, node_ptr);

    /* Scheduler unlock has work todo. */
    OS_State.scheduler_unlock_todo = 1;
  }
}
