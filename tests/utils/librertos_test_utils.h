/* Copyright (c) 2022 Djones A. Boni - MIT License */

#ifndef LIBRERTOS_TEST_UTILS_H_
#define LIBRERTOS_TEST_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "librertos.h"

void time_travel(tick_t ticks_to_the_future);
void set_tick(tick_t tick);

#ifdef __cplusplus
}
#endif

#endif /* LIBRERTOS_TEST_UTILS_H_ */
