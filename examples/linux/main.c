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

task_t task_idle;
task_t task_print1;
task_t task_print2;
task_t task_print3;

void func_task_idle(void *param) {
    (void)param;

    idle_wait_interrupt();
}

void func_task_print(void *param) {
    int value = (intptr_t)param;

    printf("func_task_print %d %u\n", TICKS_PER_SECOND * value, get_tick());
    task_delay(TICKS_PER_SECOND * value);
}

int main(void) {
    port_init();
    librertos_init();

    librertos_create_task(LOW_PRIORITY, &task_idle, &func_task_idle, NULL);
    librertos_create_task(HIGH_PRIORITY, &task_print1, &func_task_print, (void *)1);
    librertos_create_task(HIGH_PRIORITY, &task_print2, &func_task_print, (void *)2);
    librertos_create_task(HIGH_PRIORITY, &task_print3, &func_task_print, (void *)3);

    port_enable_tick_interrupt();

    printf("FUNC delay ticks\n");

    librertos_start();
    for (;;)
        librertos_sched();

    return 0;
}
