/* Copyright (c) 2022 Djones A. Boni - MIT License */

#ifndef LIBRERTOS_PROJ_H_
#define LIBRERTOS_PROJ_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define LIBRERTOS_ASSERT(expr, val, msg) \
    do \
    { \
        if (!(expr)) \
            librertos_assert((val), (msg)); \
    } while (0)

#define NUM_PRIORITIES 2
#define KERNEL_MODE kernel_mode

typedef uint16_t tick_t;
typedef int16_t difftick_t;

extern int8_t kernel_mode;

extern void librertos_assert(intptr_t val, const char *msg);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

class AssertionError
{
public:
    const char *const msg;
    AssertionError(const char *m) : msg(m) {}
};

#endif /* __cplusplus */

#endif /* LIBRERTOS_PROJ_H_ */
