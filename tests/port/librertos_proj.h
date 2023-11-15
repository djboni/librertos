/* Copyright (c) 2016-2023 Djones A. Boni - MIT License */

#ifndef LIBRERTOS_PROJ_H_
#define LIBRERTOS_PROJ_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define LIBRERTOS_ASSERT(expr, msg) \
    do { \
        if (!(expr)) \
            librertos_assert((msg)); \
    } while (0)

#define NUM_PRIORITIES 4
#define KERNEL_MODE kernel_mode

typedef uint16_t tick_t;
typedef int16_t difftick_t;

#define TICKS_PER_SECOND 10
#define TICK_PERIOD (1.0 / TICKS_PER_SECOND)

extern int8_t kernel_mode;

extern void librertos_assert(const char *msg);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

class AssertionError {
  public:
    const char *const msg;
    AssertionError(const char *m)
        : msg(m) {
    }
};

#endif /* __cplusplus */

#endif /* LIBRERTOS_PROJ_H_ */
