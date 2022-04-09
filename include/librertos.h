/* Copyright (c) 2022 Djones A. Boni - MIT License */

#ifndef LIBRERTOS_H_
#define LIBRERTOS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "librertos_port.h"
#include <stdint.h>

typedef enum
{
    FAIL = 0,
    SUCCESS = 1
} result_t;

typedef struct
{
    uint8_t count;
    uint8_t max;
} semaphore_t;

void semaphore_init(semaphore_t *sem, uint8_t init_count, uint8_t max_count);
void semaphore_init_locked(semaphore_t *sem, uint8_t max_count);
void semaphore_init_unlocked(semaphore_t *sem, uint8_t max_count);
result_t semaphore_lock(semaphore_t *sem);
result_t semaphore_unlock(semaphore_t *sem);
uint8_t semaphore_count(semaphore_t *sem);
uint8_t semaphore_max(semaphore_t *sem);

#ifdef __cplusplus
}
#endif

#endif /* LIBRERTOS_H_ */
