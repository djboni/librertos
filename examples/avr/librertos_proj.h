/* Copyright (c) 2016-2023 Djones A. Boni - MIT License */

#ifndef LIBRERTOS_PROJ_H_
#define LIBRERTOS_PROJ_H_

#ifdef __cplusplus
extern "C" {
#endif

#define __ASSERT_USE_STDERR
#include <assert.h>
#include <stdint.h>

#define LIBRERTOS_ASSERT(expr, msg) assert(expr)

#define KERNEL_MODE LIBRERTOS_PREEMPTIVE
#define NUM_PRIORITIES 2

#define F_CPU 16000000
#define TIMER_PRESCALER 64
#define TICK_PERIOD (256.0 * TIMER_PRESCALER / F_CPU)
#define TICKS_PER_SECOND ((tick_t)(1.0 / TICK_PERIOD + 0.5))

typedef uint16_t tick_t;
typedef int16_t difftick_t;

#ifdef __cplusplus
}
#endif

#endif /* LIBRERTOS_PROJ_H_ */
