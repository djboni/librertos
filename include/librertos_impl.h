/* Copyright (c) 2022 Djones A. Boni - MIT License */

#ifndef LIBRERTOS_IMPL_H_
#define LIBRERTOS_IMPL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "librertos.h"

typedef struct
{
    tick_t tick;
} librertos_t;

extern librertos_t librertos;

#ifdef __cplusplus
}
#endif

#endif /* LIBRERTOS_IMPL_H_ */
