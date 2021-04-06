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

/** Initialize event (read) struct.

Complexity: constant, O(1).
 */
void OSEventRInit(struct event_r_t *ptr) { OSListHeadInit(&ptr->list_read); }

/** Initialize event (read/write) struct.

Complexity: constant, O(1).
 */
void OSEventRwInit(struct event_rw_t *ptr) {
  OSListHeadInit(&ptr->list_read);
  OSListHeadInit(&ptr->list_write);
}

/** Pend task on an event (part 1). Must be called with interrupts disabled and
 scheduler locked.

Complexity: constant, O(1).
 */
void OSEventPrePendTask(struct task_head_list_t *list_ptr,
                        struct task_t *task_ptr) {
  /* Before finding its correct position, based on priority, put the task on the
   head position in the event list, after lowest priority (tail is higher
   priority). So the task may be unblocked by an interrupt while
   OSEventPendTask() executes. */

  struct task_list_node_t *node_ptr = &task_ptr->node_event;
  OSListInsertAfter(list_ptr, LIST_HEAD(list_ptr), node_ptr);
}

/** Pend task on an event (part 2). Must be called with interrupts enabled and
 scheduler locked. Parameter ticks_to_wait must not be zero.

Complexity: worst case linear on number of items in list, O(n).

O(n) requires interrupts to be enabled.

Used in the following functions with scheduler locked and interrupts enabled:
 - SemaphorePend()
 - MutexPend()
 - QueuePendRead()
 - QueuePendWrite()
 - FifoPendRead()
 - FifoPendWrite()
 */
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
      /* An interrupt resumed the task. */
      INTERRUPTS_ENABLE();
    }
  }
}

/** Unblock task in an event list. Must be called with scheduler locked and in a
 critical section.

Complexity: constant, O(1).
 */
void OSEventUnblockTasks(struct task_head_list_t *list_ptr) {
  /* Remove the tail (higher priority task). */
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
