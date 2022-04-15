/* Copyright (c) 2022 Djones A. Boni - MIT License */

#ifndef LIBRERTOS_CUSTOM_TESTS_H_
#define LIBRERTOS_CUSTOM_TESTS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "librertos.h"
#include "librertos_impl.h"

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

    #include <vector>

    #include "CppUTest/TestHarness.h"
    #include "CppUTestExt/MockSupport.h"

void list_tester(list_t *list, std::vector<node_t *> nodes);

/*******************************************************************************
 * LibreRTOS
 ******************************************************************************/

template <class T>
void initialize_os(T *)
{
    librertos_init();
}

template <class T>
void create_one_task(
    T *test, int priority, task_function_t func, task_parameter_t param)
{
    uint8_t task_position = test->tasks_used++;

    CHECK_TRUE_TEXT(
        task_position < sizeof(test->task) / sizeof(test->task[0]),
        "Creating too many tasks in this test.");

    librertos_create_task(priority, &test->task[task_position], func, param);
}

template <class T>
void create_N_tasks_with_different_priorities(
    T *test, int num, task_function_t func, task_parameter_t param)
{
    for (int priority = 0; priority < num; priority++)
        create_one_task(test, priority, func, param);
}

template <class T>
void taskX_is_suspended(T *test, int i)
{
    task_suspend(&test->task[i]);
}

template <class T>
void call_the_scheduler(T *)
{
    if (librertos.scheduler_lock)
        librertos_start();
    librertos_sched();
}

template <class T>
void tasks_should_be_scheduled_in_the_order(
    T *,
    std::vector<task_t *> &&expected,
    std::vector<task_t *> &actual_sequence)
{
    LONGS_EQUAL(expected.size(), actual_sequence.size());

    for (unsigned i = 0; i < expected.size(); i++)
    {
        char buff[64];
        snprintf(
            &buff[0],
            sizeof(buff),
            "Position %u. Distance %u items.",
            i,
            (unsigned)(expected[i] - actual_sequence[i]));
        POINTERS_EQUAL_TEXT(expected[i], actual_sequence[i], buff);
    }
}

/*******************************************************************************
 * Mutex mtx
 ******************************************************************************/

template <class T>
void initialize_mutex(T *test)
{
    mutex_init(&test->mtx);
}

template <class T>
void taskX_is_scheduled_and_locks_mutex(T *test, int i)
{
    set_current_task(&test->task[i]);
    mutex_lock(&test->mtx);
    set_current_task(NO_TASK_PTR);
}

template <class T>
void mutex_is_unlocked(T *test)
{
    mutex_unlock(&test->mtx);
}

template <class T>
void taskX_is_scheduled_and_suspends_on_mutex(T *test, int i)
{
    set_current_task(&test->task[i]);
    mutex_suspend(&test->mtx);
    set_current_task(NO_TASK_PTR);
}

/*******************************************************************************
 * Mutex helper
 ******************************************************************************/

template <class T>
void initialize_helper_mutex(T *test)
{
    mutex_init(&test->helper);
}

template <class T>
void interrupt_locks_helper_mutex(T *test)
{
    set_current_task(INTERRUPT_TASK_PTR);
    mutex_lock(&test->helper);
    set_current_task(NO_TASK_PTR);
}

template <class T>
void helper_mutex_is_unlocked(T *test)
{
    mutex_unlock(&test->helper);
}

template <class T>
void taskX_is_scheduled_and_suspends_helper_on_mutex(T *test, int i)
{
    set_current_task(&test->task[i]);
    mutex_suspend(&test->helper);
    set_current_task(NO_TASK_PTR);
}

#endif /* __cplusplus */

#endif /* LIBRERTOS_CUSTOM_TESTS_H_ */
