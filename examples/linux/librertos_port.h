/* Copyright (c) 2016-2023 Djones A. Boni - MIT License */

#ifndef LIBRERTOS_PORT_H_
#define LIBRERTOS_PORT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "librertos_proj.h"

#define INTERRUPTS_VAL()
void INTERRUPTS_DISABLE(void);
void INTERRUPTS_ENABLE(void);

#define CRITICAL_VAL()
void CRITICAL_ENTER(void);
void CRITICAL_EXIT(void);

void port_init(void);
void port_enable_tick_interrupt(void);
void idle_wait_interrupt(void);

#ifdef __cplusplus
}
#endif

#endif /* LIBRERTOS_PORT_H_ */
