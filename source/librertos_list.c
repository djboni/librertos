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

/** Initialize list head.

Complexity: constant, O(1).
 */
void OSListHeadInit(struct task_head_list_t *ptr) {
  /* Use the list head as a node. */
  ptr->head_ptr = LIST_HEAD(ptr);
  ptr->tail_ptr = LIST_HEAD(ptr);
  ptr->length = 0;
}

/** Initialize list node.

Complexity: constant, O(1).
 */
void OSListNodeInit(struct task_list_node_t *ptr, void *owner_ptr) {
  ptr->next_ptr = NULL;
  ptr->prev_ptr = NULL;
  ptr->value = 0;
  ptr->list_ptr = NULL;
  ptr->owner_ptr = owner_ptr;
}

/** Insert node into list. Position according to value.

Complexity: worst case linear on number of items in list, O(n).

O(n) requires interrupts to be enabled.

Used in the following functions with scheduler locked and interrupts enabled:
 - TaskDelay()
 */
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

/** Insert node into list after pos_ptr.

Complexity: constant, O(1).
 */
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

/** Remove node from list.

Complexity: constant, O(1).
 */
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
