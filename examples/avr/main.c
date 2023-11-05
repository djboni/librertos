/* Copyright (c) 2016-2023 Djones A. Boni - MIT License */

/*
 * Project tested on Atmega2560 with Arduino bootloader,
 * building on Ubuntu 22.04.
 *
 * One task blinks with frequency 1 Hz.
 *
 * Microcontroler configurations:
 *
 * - Compiler preprocessor: F_CPU=16000000
 * - Compiler preprocessor: __AVR_ATmega2560__
 * - Compiler command: -mmcu=atmega2560
 * - Linker command: -mmcu=atmega2560
 * - Build steps:
 *   - avr-size: --mcu=atmega2560
 *   - avrdude: all the command
 */

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

int main(void) {
    port_init();
    librertos_init();

    librertos_create_task(LOW_PRIORITY, &task_idle, &func_task_idle, NULL);
    librertos_create_task(HIGH_PRIORITY, &task_blink, &func_task_blink, NULL);

    port_enable_tick_interrupt();

    librertos_start();
    for (;;)
        librertos_sched();

    return 0;
}
