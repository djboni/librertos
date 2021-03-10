/*
 LibreRTOS - Portable single-stack Real Time Operating System.

 Linked list.

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

#ifndef LIBRERTOS_OSLIST_H_
#define LIBRERTOS_OSLIST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

void OS_listHeadInit(struct taskHeadList_t *list);

void OS_listNodeInit(struct taskListNode_t *node, void *owner);

void OS_listInsert(struct taskHeadList_t *list, struct taskListNode_t *node,
                   tick_t value);

void OS_listInsertAfter(struct taskHeadList_t *list, struct taskListNode_t *pos,
                        struct taskListNode_t *node);

void OS_listRemove(struct taskListNode_t *node);

#ifdef __cplusplus
}
#endif

#endif /* LIBRERTOS_OSLIST_H_ */
