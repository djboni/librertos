/*
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

#ifndef PROJDEFS_H_
#define PROJDEFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* LibreRTOS definitions. */
#define LIBRERTOS_MAX_PRIORITY 3   /* integer > 0 */
#define LIBRERTOS_PREEMPTION 0     /* boolean */
#define LIBRERTOS_PREEMPT_LIMIT 0  /* integer >= 0, < LIBRERTOS_MAX_PRIORITY */
#define LIBRERTOS_SOFTWARETIMERS 0 /* boolean */
#define LIBRERTOS_STATE_GUARDS 0   /* boolean */
#define LIBRERTOS_STATISTICS 0     /* boolean */

typedef int8_t priority_t;
typedef uint8_t schedulerLock_t;
typedef uint16_t tick_t;
typedef int16_t difftick_t;
typedef uint32_t stattime_t;
typedef uint16_t len_t;
typedef uint8_t bool_t;

#define MAX_DELAY ((tick_t)-1)

/* Assert macro. */
#define ASSERT(x)

/* Enable/disable interrupts macros. */
#define INTERRUPTS_ENABLE() __asm __volatile("sei" ::: "memory")
#define INTERRUPTS_DISABLE() __asm __volatile("cli" ::: "memory")

/* Nested critical section management macros. */
#define CRITICAL_VAL() uint8_t __istate_val
#define CRITICAL_ENTER()                                                       \
  __asm __volatile("in %0, __SREG__ \n\t"                                      \
                   "cli             \n\t"                                      \
                   : "=r"(__istate_val)::"memory")
#define CRITICAL_EXIT()                                                        \
  __asm __volatile("out __SREG__, %0 \n\t" ::"r"(__istate_val) : "memory")

/* Simulate concurrent access. For test coverage only. */
#define LIBRERTOS_TEST_CONCURRENT_ACCESS()

#ifdef __cplusplus
}
#endif

#endif /* PROJDEFS_H_ */
