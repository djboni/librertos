/* Copyright (c) 2016-2023 Djones A. Boni - MIT License */

#include "librertos.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define NUM_1000 1000
#define NUM_10000 10000
#define NUM_100000 100000

void test_scheduler_with_no_tasks(void) {
    uint32_t i;
    tick_t start, end, diff;
    float time_ms;

    start = get_tick();
    for (i = 0; i < NUM_100000; ++i) {
        librertos_sched();
    }
    end = get_tick();

    diff = end - start;
    time_ms = diff * TICK_PERIOD * 1000.0;

    printf("%s tick=%lu ms=%lu num=%luk fHz=%lu\n",
        __func__, (uint32_t)diff, (uint32_t)time_ms, (uint32_t)NUM_100000 / 1000,
        (uint32_t)(1.0 / (time_ms / 1000.0 / NUM_100000)));
}

task_t task1;

void func_run_N_times_then_suspend(void *param) {
    static uint32_t i = 0;
    (void)param;

    if (i < NUM_100000) {
        ++i;
    } else {
        /* Suspend the current task. */
        task_suspend(NULL);
    }
}

void test_scheduler_with_one_task(void) {
    uint32_t i;
    tick_t start, end, diff;
    float time_ms;

    start = get_tick();
    librertos_create_task(LOW_PRIORITY, &task1, &func_run_N_times_then_suspend, NULL);
    /* librertos_sched() is called in librertos_create_task(). */
    end = get_tick();

    diff = end - start;
    time_ms = diff * TICK_PERIOD * 1000.0;

    printf("%s tick=%lu ms=%lu num=%luk fHz=%lu\n",
        __func__, (uint32_t)diff, (uint32_t)time_ms, (uint32_t)NUM_100000 / 1000,
        (uint32_t)(1.0 / (time_ms / 1000.0 / NUM_100000)));
}

task_t task2;

void func_delay_and_resume_N_times_then_suspend(void *param) {
    static uint32_t i = 0;
    (void)param;
    task_delay(1000);
    task_resume(&task2);

    if (i < NUM_10000) {
        ++i;
    } else {
        /* Suspend the current task. */
        task_suspend(NULL);
    }
}

void test_task_delay_resume(void) {
    uint32_t i;
    tick_t start, end, diff;
    float time_ms;

    start = get_tick();
    librertos_create_task(LOW_PRIORITY, &task2, &func_delay_and_resume_N_times_then_suspend, NULL);
    /* librertos_sched() is called in librertos_create_task(). */
    end = get_tick();

    diff = end - start;
    time_ms = diff * TICK_PERIOD * 1000.0;

    printf("%s tick=%lu ms=%lu num=%luk fHz=%lu\n",
        __func__, (uint32_t)diff, (uint32_t)time_ms, (uint32_t)NUM_10000 / 1000,
        (uint32_t)(1.0 / (time_ms / 1000.0 / NUM_10000)));
}

queue_t que1;
uint8_t buff_que1[32];

void test_queue_write_read(void) {
    uint32_t i;
    tick_t start, end, diff;
    float time_ms;

    queue_init(&que1, &buff_que1, sizeof(buff_que1) / sizeof(buff_que1[0]), sizeof(buff_que1[0]));

    start = get_tick();
    for (i = 0; i < NUM_100000; ++i) {
        uint8_t data;
        queue_write(&que1, &data);
        queue_read(&que1, &data);
    }
    end = get_tick();

    diff = end - start;
    time_ms = diff * TICK_PERIOD * 1000.0;

    printf("%s tick=%lu ms=%lu num=%luk fHz=%lu\n",
        __func__, (uint32_t)diff, (uint32_t)time_ms, (uint32_t)NUM_100000 / 1000,
        (uint32_t)(1.0 / (time_ms / 1000.0 / NUM_100000)));
}

queue_t que2;
uint8_t buff_que2[32];
task_t task3;
task_t task4;

void func_send_data_to_queue_N_times_then_suspend(void *param) {
    static uint32_t i = 0;
    (void)param;

    if (i < NUM_10000) {
        uint8_t data = i++;
        queue_write(&que2, &data);
    } else {
        /* Suspend the current task. */
        task_suspend(NULL);
    }
}

void func_read_data_from_queue_2(void *param) {
    uint8_t data;
    queue_read_suspend(&que2, &data, MAX_DELAY);
}

void test_queue_write_read_suspending_task(void) {
    uint32_t i;
    tick_t start, end, diff;
    float time_ms;

    queue_init(&que2, &buff_que2, sizeof(buff_que2) / sizeof(buff_que2[0]), sizeof(buff_que2[0]));

    start = get_tick();
    scheduler_lock();
    librertos_create_task(LOW_PRIORITY, &task3, &func_send_data_to_queue_N_times_then_suspend, NULL);
    librertos_create_task(HIGH_PRIORITY, &task4, &func_read_data_from_queue_2, NULL);
    scheduler_unlock();
    /* librertos_sched() is called in scheduler_unlock(). */
    end = get_tick();

    diff = end - start;
    time_ms = diff * TICK_PERIOD * 1000.0;

    printf("%s tick=%lu ms=%lu num=%luk fHz=%lu\n",
        __func__, (uint32_t)diff, (uint32_t)time_ms, (uint32_t)NUM_10000 / 1000,
        (uint32_t)(1.0 / (time_ms / 1000.0 / NUM_10000)));
}

queue_t que3;
uint8_t buff_que3[32];
task_t task5;
task_t task6;

void func_send_data_to_queue_N_times_then_delay(void *param) {
    static uint32_t i = 0;
    (void)param;

    if (i < NUM_10000) {
        uint8_t data = i++;
        queue_write(&que3, &data);
    } else {
        /* Suspend the current task. */
        task_suspend(NULL);
    }
}

void func_read_data_from_queue_3(void *param) {
    uint8_t data;
    queue_read_suspend(&que3, &data, 1000);
}

void test_queue_write_read_delaying_task(void) {
    uint32_t i;
    tick_t start, end, diff;
    float time_ms;

    queue_init(&que3, &buff_que3, sizeof(buff_que3) / sizeof(buff_que3[0]), sizeof(buff_que3[0]));

    start = get_tick();
    scheduler_lock();
    librertos_create_task(LOW_PRIORITY, &task5, &func_send_data_to_queue_N_times_then_delay, NULL);
    librertos_create_task(HIGH_PRIORITY, &task6, &func_read_data_from_queue_3, NULL);
    scheduler_unlock();
    /* librertos_sched() is called in scheduler_unlock(). */
    end = get_tick();

    diff = end - start;
    time_ms = diff * TICK_PERIOD * 1000.0;

    printf("%s tick=%lu ms=%lu num=%luk fHz=%lu\n",
        __func__, (uint32_t)diff, (uint32_t)time_ms, (uint32_t)NUM_10000 / 1000,
        (uint32_t)(1.0 / (time_ms / 1000.0 / NUM_10000)));
}

#define ADD_TASKS_TO_DELAY_LIST 0

#if ADD_TASKS_TO_DELAY_LIST
task_t task_x1[ADD_TASKS_TO_DELAY_LIST];
task_t task_x2;

void func_task_x1(void *param) {
    task_delay(999);
}

void func_task_x2(void *param) {
    task_delay(MAX_DELAY - 1);
}
#endif /* ADD_TASKS_TO_DELAY_LIST */

int main(void) {
    uint32_t i;
    (void)i;

    port_init();
    serial_init(115200);
    librertos_init();
    port_enable_tick_interrupt();
    librertos_start();

    printf("begin\n");
    test_scheduler_with_no_tasks();
    test_scheduler_with_one_task();
    test_queue_write_read();

#if ADD_TASKS_TO_DELAY_LIST
    for (i = 0; i < ADD_TASKS_TO_DELAY_LIST; ++i)
        librertos_create_task(LOW_PRIORITY, &task_x1[i], &func_task_x1, NULL);
    librertos_create_task(LOW_PRIORITY, &task_x2, &func_task_x2, NULL);
#endif /* ADD_TASKS_TO_DELAY_LIST */

    test_task_delay_resume();
    test_queue_write_read_suspending_task();
    test_queue_write_read_delaying_task();
    printf("end\n");

    while (1) {
        librertos_sched();
    }

    return 0;
}

/*
 * ADD_TASKS_TO_DELAY_LIST = 0
 * test_scheduler_with_no_tasks tick=922 ms=944 num=100k fHz=105917
 * test_scheduler_with_one_task tick=1638 ms=1677 num=100k fHz=59619
 * test_queue_write_read tick=2753 ms=2819 num=100k fHz=35472
 * test_task_delay_resume tick=810 ms=829 num=10k fHz=12056
 * test_queue_write_read_suspending_task tick=1458 ms=1492 num=10k fHz=6697
 * test_queue_write_read_delaying_task tick=1750 ms=1791 num=10k fHz=5580
 *
 * ADD_TASKS_TO_DELAY_LIST = 1
 * test_scheduler_with_no_tasks tick=922 ms=944 num=100k fHz=105917
 * test_scheduler_with_one_task tick=1638 ms=1677 num=100k fHz=59619
 * test_queue_write_read tick=2753 ms=2819 num=100k fHz=35472
 * test_task_delay_resume tick=833 ms=852 num=10k fHz=11723
 * test_queue_write_read_suspending_task tick=1458 ms=1492 num=10k fHz=6697
 * test_queue_write_read_delaying_task tick=1772 ms=1814 num=10k fHz=5511
 *
 * ADD_TASKS_TO_DELAY_LIST = 2
 * test_scheduler_with_no_tasks tick=922 ms=944 num=100k fHz=105917
 * test_scheduler_with_one_task tick=1638 ms=1677 num=100k fHz=59619
 * test_queue_write_read tick=2753 ms=2819 num=100k fHz=35472
 * test_task_delay_resume tick=851 ms=871 num=10k fHz=11475
 * test_queue_write_read_suspending_task tick=1459 ms=1494 num=10k fHz=6693
 * test_queue_write_read_delaying_task tick=1793 ms=1836 num=10k fHz=5446
 *
 * ADD_TASKS_TO_DELAY_LIST = 10
 * test_scheduler_with_no_tasks tick=922 ms=944 num=100k fHz=105917
 * test_scheduler_with_one_task tick=1638 ms=1677 num=100k fHz=59619
 * test_queue_write_read tick=2753 ms=2819 num=100k fHz=35472
 * test_task_delay_resume tick=1022 ms=1046 num=10k fHz=9555
 * test_queue_write_read_suspending_task tick=1459 ms=1494 num=10k fHz=6693
 * test_queue_write_read_delaying_task tick=1966 ms=2013 num=10k fHz=4967
 *
 * ADD_TASKS_TO_DELAY_LIST = 100
 * test_scheduler_with_no_tasks tick=922 ms=944 num=100k fHz=105917
 * test_scheduler_with_one_task tick=1638 ms=1677 num=100k fHz=59619
 * test_queue_write_read tick=2753 ms=2819 num=100k fHz=35472
 * test_task_delay_resume tick=2974 ms=3045 num=10k fHz=3283
 * test_queue_write_read_suspending_task tick=1508 ms=1544 num=10k fHz=6475
 * test_queue_write_read_delaying_task tick=3975 ms=4070 num=10k fHz=2456
 */
