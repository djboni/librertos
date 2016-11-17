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

#ifndef PROJDEFS_H_
#define PROJDEFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>

/* LibreRTOS definitions. */
#define LIBRERTOS_MAX_PRIORITY       3  /* integer > 0 */
#define LIBRERTOS_NUM_TASKS          2  /* integer > 0 */
#define LIBRERTOS_PREEMPTION         0  /* boolean */
#define LIBRERTOS_TICK               1  /* boolean */
#define LIBRERTOS_QUEUE_1CRITICAL    0  /* boolean */
#define LIBRERTOS_FIFO_1CRITICAL     0  /* boolean */

typedef int8_t priority_t;
typedef int8_t schedulerLock_t;
typedef uint16_t tick_t;
#define MAX_DELAY ((tick_t)0xFFFFU)

/* Assert macro. */
#define ASSERT(x) assert(x)

/* Enable/disable interrupts macros. */
#define INTERRUPTS_ENABLE()
#define INTERRUPTS_DISABLE()

/* Nested critical section management macros. */
#define CRITICAL_VAL()
#define CRITICAL_ENTER()
#define CRITICAL_EXIT()

#ifdef __cplusplus
}
#endif

#endif /* PROJDEFS_H_ */
