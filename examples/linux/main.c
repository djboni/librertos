/* Copyright (c) 2016-2023 Djones A. Boni - MIT License */

/*
 * Project tested on Ubuntu 22.04.
 *
 * Three tasks print the task number and the tick count.
 */

#include "librertos.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define NUM_TASKS_PRINT 5

task_t task_idle;
task_t task_print[NUM_TASKS_PRINT];

void func_task_idle(void *param) {
    (void)param;

    idle_wait_interrupt();
}

void func_task_print(void *param) {
    int value = (intptr_t)param * TICKS_PER_SECOND;

    printf("func_task_print %d %u\n", value, get_tick());

    task_delay(value);
}

int main(void) {
    uintptr_t i;

    port_init();
    librertos_init();

    librertos_create_task(LOW_PRIORITY, &task_idle, &func_task_idle, NULL);
    for (i = 0; i < NUM_TASKS_PRINT; ++i)
        librertos_create_task(HIGH_PRIORITY, &task_print[i], &func_task_print, (void *)(i + 1));

    port_enable_tick_interrupt();

    printf("FUNC delay tick\n");

    librertos_start();
    for (;;)
        librertos_sched();

    return 0;
}
