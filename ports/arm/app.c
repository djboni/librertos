/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos.h"
#include <stddef.h>
#include <stdint.h>

task_t task_idle;
task_t task_blink;

void func_task_idle(void *param) {
    (void)param;

    idle_wait_interrupt();
}

void func_task_blink(void *param) {
    (void)param;

    led_toggle();
    task_delay(0.5 * TICKS_PER_SECOND);
}

void setup(void) {
    port_init();
    librertos_init();

    librertos_create_task(LOW_PRIORITY, &task_idle, &func_task_idle, NULL);
    librertos_create_task(HIGH_PRIORITY, &task_blink, &func_task_blink, NULL);
}

void pre_loop(void) {
    port_enable_tick_interrupt();
    librertos_start();
}

void loop(void) {
    librertos_sched();
}

#if 0
int main(void) {
	setup();

	/* hardware_init(); */

	pre_loop();
	for(;;)
		loop();
}
#endif
