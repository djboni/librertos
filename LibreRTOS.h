/*
 Copyright 2016 Djones A. Boni

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

#ifndef LIBRERTOS_H_
#define LIBRERTOS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "projdefs.h"

typedef void* taskParameter_t;
typedef void(*taskFunction_t)(taskParameter_t);

void OS_init(void);

void OS_schedule(void);
int8_t OS_schedulerIsLocked(void);
void OS_schedulerLock(void);
void OS_schedulerUnlock(void);

void OS_taskCreate(
        int8_t priority,
        taskFunction_t function,
        taskParameter_t parameter);
void OS_taskDelete(int8_t priority);

#ifdef __cplusplus
}
#endif

#endif /* LIBRERTOS_H_ */
