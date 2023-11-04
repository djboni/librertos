/* Copyright (c) 2016-2023 Djones A. Boni - MIT License */

#ifndef LIBRERTOS_PROJ_H_
#define LIBRERTOS_PROJ_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <stdint.h>

#define LIBRERTOS_ASSERT(expr, val, msg) assert(expr)

#define NUM_PRIORITIES 2

#define TICKS_PER_SECOND 100
#define TICK_PERIOD (1.0 / TICKS_PER_SECOND)

typedef uint16_t tick_t;
typedef int16_t difftick_t;

#ifdef __cplusplus
}
#endif

#endif /* LIBRERTOS_PROJ_H_ */
