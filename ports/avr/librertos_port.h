/* Copyright (c) 2022 Djones A. Boni - MIT License */

#ifndef LIBRERTOS_PORT_H_
#define LIBRERTOS_PORT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "librertos_proj.h"

#define INTERRUPTS_VAL()
#define INTERRUPTS_DISABLE() __asm __volatile("cli" :: \
                                                  : "memory")
#define INTERRUPTS_ENABLE() __asm __volatile("sei" :: \
                                                 : "memory")

#define CRITICAL_VAL() uint8_t __istate_val
#define CRITICAL_ENTER() \
    __asm __volatile( \
        "in %0, __SREG__ \n\t" \
        "cli             \n\t" \
        : "=r"(__istate_val)::"memory")
#define CRITICAL_EXIT() \
    __asm __volatile("out __SREG__, %0 \n\t" ::"r"(__istate_val) \
                     : "memory")

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
