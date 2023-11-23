/* Copyright (c) 2016-2023 Djones A. Boni - MIT License */

#ifndef LIBRERTOS_PROJ_H_
#define LIBRERTOS_PROJ_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <stdint.h>

#define LIBRERTOS_ASSERT(expr, msg) assert(expr)

#define KERNEL_MODE LIBRERTOS_PREEMPTIVE
#define NUM_PRIORITIES 2

#define TICKS_PER_SECOND 1000
#define TICK_PERIOD (1.0 / TICKS_PER_SECOND)

typedef uint32_t tick_t;
typedef int32_t difftick_t;

#define LIBRERTOS_DISABLE_TIMERS 0
#define LIBRERTOS_DISABLE_SEMAPHORES 0
#define LIBRERTOS_DISABLE_MUTEXES 0
#define LIBRERTOS_DISABLE_QUEUES 0

#ifdef __cplusplus
}
#endif

#endif /* LIBRERTOS_PROJ_H_ */
