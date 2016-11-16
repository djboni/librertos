/*
 Copyright 2016 Djones A. Boni

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

#ifndef LIBRERTOS_OSEVENT_H_
#define LIBRERTOS_OSEVENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "LibreRTOS.h"
#include "OSlist.h"
#include <stddef.h>

void OS_eventRInit(struct eventR_t* o);
void OS_eventRwInit(struct eventRw_t* o);
void OS_eventPendTask(
        struct taskHeadList_t* list,
        priority_t priority,
        tick_t ticksToWait);
void OS_eventUnblockTasks(struct taskHeadList_t* list);

struct taskListNode_t* OS_getTaskEventNode(priority_t priority);

#ifdef __cplusplus
}
#endif

#endif /* LIBRERTOS_OSEVENT_H_ */
