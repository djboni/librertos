/* Copyright (c) 2016-2023 Djones A. Boni - MIT License */

#ifndef LIBRERTOS_PROJ_H_
#define LIBRERTOS_PROJ_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define LIBRERTOS_ASSERT(expr, val, msg) \
    do { \
        if (!(expr)) { \
            while (1) { \
            } \
        } \
    } while (0)

#define KERNEL_MODE LIBRERTOS_PREEMPTIVE
#define NUM_PRIORITIES 2

#define TICKS_PER_SECOND 1000
#define TICK_PERIOD (1.0 / TICKS_PER_SECOND)

typedef uint32_t tick_t;
typedef int32_t difftick_t;

#ifdef __cplusplus
}
#endif

#endif /* LIBRERTOS_PROJ_H_ */
