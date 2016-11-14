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

#include "OSlist.h"

#define LIST_HEAD(x) ((struct taskListNode_t*)x)

void OS_listHeadInit(struct taskHeadList_t* list)
{
    /* Use the list head as a node. */
    list->ListHead = (struct taskListNode_t*)list;
    list->ListTail = (struct taskListNode_t*)list;
    list->TickToWakeup_Zero = 0U;
    list->ListLength = 0;
}

void OS_listNodeInit(struct taskListNode_t* node, priority_t priority)
{
    node->ListNext = NULL;
    node->ListPrevious = NULL;
    node->TickToWakeup = 0;
    node->ListInserted = NULL;
    node->TaskPriority = priority;
}

void OS_listInsert(
        struct taskHeadList_t* list,
        struct taskListNode_t* node,
        tick_t tickToWakeup)
{
    struct taskListNode_t* pos = list->ListHead;

    while(pos != LIST_HEAD(list))
    {
        if(tickToWakeup >= pos->TickToWakeup)
        {
            /* Not here. */
            pos = pos->ListNext;
        }
        else
        {
            /* Insert here. */
            break;
        }
    }

    node->TickToWakeup = tickToWakeup;
    node->ListInserted = list;

    node->ListNext = pos;
    node->ListPrevious = pos->ListPrevious;

    pos->ListPrevious->ListNext = node;
    pos->ListPrevious = node;

    ++list->ListLength;
}

void OS_listRemove(struct taskListNode_t* node)
{
    struct taskListNode_t* next = node->ListNext;
    struct taskListNode_t* previous = node->ListPrevious;

    next->ListPrevious = previous;
    previous->ListNext = next;

    node->ListNext = NULL;
    node->ListPrevious = NULL;

    --node->ListInserted->ListLength;
    node->ListInserted = NULL;
}
