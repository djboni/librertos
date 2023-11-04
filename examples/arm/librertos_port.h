/* Copyright (c) 2022 Djones A. Boni - MIT License */

#ifndef LIBRERTOS_PORT_H_
#define LIBRERTOS_PORT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "librertos_proj.h"
#include <cmsis_gcc.h>

#define INTERRUPTS_VAL()
#define INTERRUPTS_DISABLE() __disable_irq()
#define INTERRUPTS_ENABLE() __enable_irq()

#define CRITICAL_VAL() uint32_t __istate_val
#define CRITICAL_ENTER() \
    __istate_val = __get_PRIMASK(); \
    __disable_irq()
#define CRITICAL_EXIT() __set_PRIMASK(__istate_val)

void port_init(void);
void idle_wait_interrupt(void);
void port_enable_tick_interrupt(void);

void led_config(void);
void led_on(void);
void led_off(void);
void led_toggle(void);

#ifdef __cplusplus
}
#endif

#endif /* LIBRERTOS_PORT_H_ */
