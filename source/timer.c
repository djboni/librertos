/*
 LibreRTOS - Portable single-stack Real Time Operating System.

 Software timers.

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

#if (LIBRERTOS_SOFTWARETIMERS != 0)

/* Execute a timer. Used by timer task function. */
static void OSTimerExecute(struct timer_t *ptr) {
  timer_function_t function;
  timer_parameter_t parameter;

  INTERRUPTS_DISABLE();
  function = ptr->function;
  parameter = ptr->parameter;
  INTERRUPTS_ENABLE();

  function(ptr, parameter);
}

/* Insert timer into ordered list. Used by timer task function. */
static void OSTimerInsertInOrderedList(struct timer_t *ptr,
                                       tick_t tick_to_wakeup) {
  struct task_list_node_t *pos_ptr;
  struct task_list_node_t *const node_ptr = &ptr->node_timer;
  struct task_head_list_t *const list_ptr = &OS_State.timer_list;

  INTERRUPTS_DISABLE();

  for (;;) {
    if (tick_to_wakeup > OS_State.task_timer_last_run) {
      pos_ptr = OS_State.timer_index_ptr;
    } else {
      pos_ptr = list_ptr->head_ptr;
    }

    while (pos_ptr != (struct task_list_node_t *)list_ptr) {
      INTERRUPTS_ENABLE();

      /* For test coverage only. This macro is used as a deterministic
       way to create a concurrent access. */
      LIBRERTOS_TEST_CONCURRENT_ACCESS();

      if (pos_ptr->value >= tick_to_wakeup) {
        /* Found where to insert. Break while(). */
        INTERRUPTS_DISABLE();
        break;
      }

      INTERRUPTS_DISABLE();
      if (pos_ptr->list_ptr != list_ptr) {
        /* This position was removed from the list. Break while(). */
        break;
      }

      pos_ptr = pos_ptr->next_ptr;
    }

    if (pos_ptr != (struct task_list_node_t *)list_ptr &&
        pos_ptr->list_ptr != list_ptr &&
        node_ptr->list_ptr == &OS_State.timer_unordered_list) {
      /* This pos was removed from the list and node was not
       removed. Must restart to find where to insert node.
       Continue for(;;). */
      continue;
    } else {
      /* Found where to insert. Insert before pos.
       OR
       Item node was removed from the list. Nothing to insert.
       Break for(;;). */
      break;
    }
  }

  if (node_ptr->list_ptr == &OS_State.timer_unordered_list) {
    /* Timer was not removed from list. */

    /* Now insert in the right position. */
    OSListRemove(node_ptr);
    OSListInsertAfter(list_ptr, pos_ptr->prev_ptr, node_ptr);
    node_ptr->value = tick_to_wakeup;

    if (tick_to_wakeup > OS_State.task_timer_last_run &&
        OS_State.timer_index_ptr->prev_ptr == &ptr->node_timer) {
      OS_State.timer_index_ptr = &ptr->node_timer;
    }
  }

  INTERRUPTS_ENABLE();
}

/* Timer task function. Used by timer task create function. */
static void OSTimerFunction(task_parameter_t param) {
  uint8_t change_index;
  (void)param;

  {
    tick_t now = GetTickCount();
    if (now >= OS_State.task_timer_last_run) {
      OS_State.task_timer_last_run = now;
      change_index = 0;
    } else {
      OS_State.task_timer_last_run = MAX_DELAY;
      change_index = 1;
    }
  }

  INTERRUPTS_DISABLE();

  /* Insert timers into ordered list; execute one-shot timers. */
  while (OS_State.timer_unordered_list.length != 0) {
    struct task_list_node_t *node_ptr = OS_State.timer_unordered_list.head_ptr;
    struct timer_t *timer_ptr = (struct timer_t *)node_ptr->owner_ptr;

    if (timer_ptr->type == TIMERTYPE_NOPERIOD) {
      /* Execute one-shot timer. */
      OSListRemove(node_ptr);
      INTERRUPTS_ENABLE();
      OSTimerExecute(timer_ptr);
    } else {
      /* Insert timer into ordered list. */
      tick_t tick_to_wakeup;
      INTERRUPTS_ENABLE();
      tick_to_wakeup =
          (tick_t)(OS_State.task_timer_last_run + timer_ptr->period);
      OSTimerInsertInOrderedList(timer_ptr, tick_to_wakeup);
    }
    INTERRUPTS_DISABLE();
  }

  INTERRUPTS_ENABLE();

  /* Execute ready timer. */
  while (OS_State.timer_index_ptr !=
             (struct task_list_node_t *)&OS_State.timer_list &&
         OS_State.timer_index_ptr->value <= OS_State.task_timer_last_run) {
    struct task_list_node_t *node_ptr = OS_State.timer_index_ptr;
    struct timer_t *timer_ptr = (struct timer_t *)node_ptr->owner_ptr;
    OS_State.timer_index_ptr = node_ptr->next_ptr;

    if (timer_ptr->type == TIMERTYPE_AUTO) {
      TimerReset(timer_ptr);
    } else {
      INTERRUPTS_DISABLE();
      OSListRemove(node_ptr);
      INTERRUPTS_ENABLE();
    }

    /* Execute timer. */
    OSTimerExecute(timer_ptr);
  }

  INTERRUPTS_DISABLE();

  if (change_index != 0) {
    /* Change index. Run timer task again. */
    INTERRUPTS_ENABLE();
    OS_State.timer_index_ptr = OS_State.timer_list.head_ptr;
    OS_State.task_timer_last_run = 0;
  } else if (OS_State.timer_unordered_list.length == 0 &&
             (OS_State.timer_index_ptr ==
                  (struct task_list_node_t *)&OS_State.timer_list ||
              OS_State.timer_index_ptr->value > OS_State.tick)) {
    /* No timer is ready. Block timer task. */
    tick_t ticks_to_sleep;
    struct timer_t *next_timer_ptr;

    SchedulerLock();

    if (OS_State.timer_index_ptr !=
        (struct task_list_node_t *)&OS_State.timer_list) {
      next_timer_ptr = (struct timer_t *)OS_State.timer_index_ptr->owner_ptr;
      ticks_to_sleep = (tick_t)(next_timer_ptr->node_timer.value -
                                OS_State.task_timer_last_run);
      INTERRUPTS_ENABLE();
    } else if (OS_State.timer_list.head_ptr !=
               (struct task_list_node_t *)&OS_State.timer_list) {
      next_timer_ptr =
          (struct timer_t *)OS_State.timer_list.head_ptr->owner_ptr;
      ticks_to_sleep =
          (tick_t)(next_timer_ptr->node_timer.value - OS_State.tick);
      INTERRUPTS_ENABLE();
    } else {
      ticks_to_sleep = MAX_DELAY;
      TaskDelay(MAX_DELAY);
    }

    if ((difftick_t)ticks_to_sleep > 0) {
      /* Delay task only if timer wakeup time is not in the past.
       If there is not timer the timer task is already delayed. */
      TaskDelay(ticks_to_sleep);
    }

    SchedulerUnlock();
  } else {
    /* A timer is ready. Run timer task again. */
    INTERRUPTS_ENABLE();
  }
}

/** Create timer task.  */
void LibrertosTimerTaskCreate(priority_t priority) {
  LibrertosTaskCreate(&OS_State.task_timer_tcb, priority, &OSTimerFunction, NULL);
}

/** Initialize timer structure.

 This function does not start the timer.

 The types can be of three types: TIMERTYPE_ONESHOT, TIMERTYPE_AUTO or
 TIMERTYPE_NOPERIOD.

 An one-shot timer will run once after its period has timed-out after a reset or
 start.

 An auto timer will run multiple times after after a reset or start. The auto
 timer resets itself when it runs.

 A no-period timer will run once when the timer task is scheduled. It is
 equivalent to a timer with period equal to zero, but avoids being inserted into
 the ordered timers list. It does not use the period information, but user
 should initialize with value 0 as its period.

 @param type Define the type of the timer. The types can be TIMERTYPE_ONESHOT,
 TIMERTYPE_AUTO or TIMERTYPE_NOPERIOD.
 @param period Period of the timer. After a reset or start the timer will run
 only after its period has timed-out.
 @param function Function pointer to the timer function. It will be called by
 timer task when the timer runs. The prototype must be
 void timerFunction(struct timer_t* timer, void* param);
 @param parameter Parameter passed to the timer function.
 */
void TimerInit(struct timer_t *ptr, enum timer_type_t type, tick_t period,
               timer_function_t function, timer_parameter_t parameter) {
  ptr->type = type;
  ptr->period = period;
  ptr->function = function;
  ptr->parameter = parameter;
  OSListNodeInit(&ptr->node_timer, ptr);
}

/** Start a timer.

 Reset the timer only if it is not running.
 */
void TimerStart(struct timer_t *ptr) {
  if (!TimerIsRunning(ptr)) {
    TimerReset(ptr);
  }
}

/** Reset a timer.

 For default and auto timers:
 The timer run only after 'period' ticks have passed after now.

 For one-shot timer:
 The timer run as soon as the timer task is scheduled.
 */
void TimerReset(struct timer_t *ptr) {
  CRITICAL_VAL();
  CRITICAL_ENTER();

  if (ptr->node_timer.list_ptr != NULL) {
    /* If timer is running. */

    if (OS_State.timer_index_ptr == &ptr->node_timer) {
      OS_State.timer_index_ptr = OS_State.timer_index_ptr->next_ptr;
    }

    OSListRemove(&ptr->node_timer);
  }

  OSListInsertAfter(&OS_State.timer_unordered_list,
                    (struct task_list_node_t *)&OS_State.timer_unordered_list,
                    &ptr->node_timer);

  CRITICAL_EXIT();

  TaskResume(&OS_State.task_timer_tcb);
}

/** Stop a timer.

 The timer will not run again until it is started or reset.
 */
void TimerStop(struct timer_t *ptr) {
  CRITICAL_VAL();
  CRITICAL_ENTER();

  if (ptr->node_timer.list_ptr != NULL) {
    /* If timer is running. */

    if (OS_State.timer_index_ptr == &ptr->node_timer) {
      OS_State.timer_index_ptr = OS_State.timer_index_ptr->next_ptr;
    }

    OSListRemove(&ptr->node_timer);
  }

  CRITICAL_EXIT();
}

/** Check if a timer is running.

 A timer is running if it has been started or reset and has not stopped.

 @return 1 if timer is running, 0 if it is stopped.
 */
bool_t TimerIsRunning(const struct timer_t *ptr) {
  bool_t x;
  CRITICAL_VAL();
  CRITICAL_ENTER();

  x = ptr->node_timer.list_ptr != NULL;

  CRITICAL_EXIT();
  return x;
}

#endif
