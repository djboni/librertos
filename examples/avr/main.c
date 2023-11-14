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
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define NUM_TASKS_PRINT 2

task_t task_idle;
task_t task_blink;
task_t task_print[NUM_TASKS_PRINT];

void func_task_idle(void *param) {
    (void)param;
    idle_wait_interrupt();
}

void func_task_blink(void *param) {
    static uint8_t led_value = 0;
    (void)param;

    led_value = !led_value;
    led_write(led_value);

    task_delay(0.5 * TICKS_PER_SECOND);
}

void func_task_print(void *param) {
    int value = (intptr_t)param * TICKS_PER_SECOND;

    printf("func_task_print %" PRIdPTR " %u\n", (intptr_t)param, get_tick());

    task_delay(value);
}

int main(void) {
    uintptr_t i;

    port_init();
    serial_init(115200);
    librertos_init();

    librertos_create_task(LOW_PRIORITY, &task_idle, &func_task_idle, NULL);
    librertos_create_task(HIGH_PRIORITY, &task_blink, &func_task_blink, NULL);
    for (i = 0; i < NUM_TASKS_PRINT; ++i)
        librertos_create_task(HIGH_PRIORITY, &task_print[i], &func_task_print, (void *)(i + 1));

    port_enable_tick_interrupt();

    librertos_start();
    while (1) {
        librertos_sched();
    }

    return 0;
}
