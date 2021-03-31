/*
 LibreRTOS - Portable single-stack Real Time Operating System.

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

#ifndef LIBRERTOS_IMPL_H_
#define LIBRERTOS_IMPL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "librertos.h"
#include <stddef.h>

struct librertos_state_t {
#if (LIBRERTOS_STATE_GUARDS != 0)
  uint32_t guard_start;
#endif

  volatile scheduler_lock_t scehduler_lock; /* Scheduler lock. Controls if
                                             another task can be scheduled. */
  volatile scheduler_lock_t scheduler_unlock_todo; /* Flags the schedule unlock
                                                   function has work to do. */
  struct task_t *volatile current_tcb_ptr; /* Current task control block. */

#if (LIBRERTOS_PREEMPTION != 0)
  volatile bool_t higher_ready_task; /* Higher priority task is ready to run. */
#endif

  struct task_t *task_ptr[LIBRERTOS_MAX_PRIORITY]; /* Task priorities. */

  tick_t tick;          /* OS tick. */
  tick_t delayed_ticks; /* OS delayed tick (scheduler was locked). */
  struct task_head_list_t
      *blocked_task_list_not_overflowed_ptr; /* List with blocked tasks (not
                                         overflowed). */
  struct task_head_list_t
      *blocked_task_list_overflowed_ptr; /* List with blocked tasks
                                            (overflowed). */

  struct task_head_list_t
      pending_ready_task_list; /* List with ready tasks not removed from list of
                               blocked tasks. */
  struct task_head_list_t
      blocked_task_list_1; /* List with blocked tasks number 1. */
  struct task_head_list_t
      blocked_task_list_2; /* List with blocked tasks number 2. */

#if (LIBRERTOS_SOFTWARETIMERS != 0)
  tick_t task_timer_last_run;
  struct task_t task_timer_tcb; /* Task control block of timer task. */
  struct task_list_node_t
      *timer_index_ptr; /* Points to next timer to be run in TimerList. */
  struct task_head_list_t
      timer_list; /* List of running timers ordered by wakeup time. */
  struct task_head_list_t timer_unordered_list; /* List of running timers to be
                                               ordered by wakeup time. */
#endif

#if (LIBRERTOS_STATISTICS != 0)
  stattime_t total_run_time;
  stattime_t no_task_run_time;
#endif

#if (LIBRERTOS_STATE_GUARDS != 0)
  uint32_t guard_end;
#endif
};

extern struct librertos_state_t OS_State;

void OSListHeadInit(struct task_head_list_t *ptr);

void OSListNodeInit(struct task_list_node_t *ptr, void *owner_ptr);

void OSListInsert(struct task_head_list_t *ptr,
                  struct task_list_node_t *node_ptr, tick_t value);

void OSListInsertAfter(struct task_head_list_t *ptr,
                       struct task_list_node_t *pos_ptr,
                       struct task_list_node_t *node_ptr);

void OSListRemove(struct task_list_node_t *ptr);

void OSEventRInit(struct event_r_t *ptr);

void OSEventRwInit(struct event_rw_t *ptr);

void OSEventPrePendTask(struct task_head_list_t *list_ptr,
                        struct task_t *task_ptr);

void OSEventPendTask(struct task_head_list_t *list_ptr, struct task_t *task_ptr,
                     tick_t ticks_to_wait);

void OSEventUnblockTasks(struct task_head_list_t *list_ptr);

#ifdef __cplusplus
}
#endif

#endif /* LIBRERTOS_EVENT_H_ */
