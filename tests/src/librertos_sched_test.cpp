/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos.h"
#include "librertos_impl.h"
#include "tests/utils/librertos_test_utils.h"

/*
 * Main file: src/librertos.c
 * Also compile: tests/mocks/librertos_assert.cpp
 * Also compile: tests/utils/librertos_test_utils.cpp
 */

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

TEST_GROUP (Scheduler)
{
    int param1, param2;
    task_t task1, task2;

    void setup()
    {
        param1 = 0;
        param2 = 0;

        librertos_init();
    }
    void teardown() {}
};

TEST(Scheduler, NoTask_DoNothing)
{
    librertos_sched();
}

static void increment_param(void *param)
{
    int *p = (int *)param;
    *p += 1;
}

TEST(Scheduler, OneTask_RunTask1)
{
    librertos_create_task(LOW_PRIORITY, &task1, &increment_param, &param1);

    librertos_sched();
    LONGS_EQUAL(1, param1);

    librertos_sched();
    LONGS_EQUAL(2, param1);
}

TEST(Scheduler, TwoTasks_SamePriorityRunCooperatively)
{
    librertos_create_task(LOW_PRIORITY, &task1, &increment_param, &param1);
    librertos_create_task(LOW_PRIORITY, &task2, &increment_param, &param2);

    librertos_sched();
    LONGS_EQUAL(1, param1);
    LONGS_EQUAL(0, param2);

    librertos_sched();
    LONGS_EQUAL(1, param1);
    LONGS_EQUAL(1, param2);

    librertos_sched();
    LONGS_EQUAL(2, param1);
    LONGS_EQUAL(1, param2);

    librertos_sched();
    LONGS_EQUAL(2, param1);
    LONGS_EQUAL(2, param2);
}

TEST(Scheduler, TwoTasks_SamePriorityRunCooperatively_2)
{
    librertos_create_task(HIGH_PRIORITY, &task1, &increment_param, &param1);
    librertos_create_task(HIGH_PRIORITY, &task2, &increment_param, &param2);

    librertos_sched();
    LONGS_EQUAL(1, param1);
    LONGS_EQUAL(0, param2);

    librertos_sched();
    LONGS_EQUAL(1, param1);
    LONGS_EQUAL(1, param2);

    librertos_sched();
    LONGS_EQUAL(2, param1);
    LONGS_EQUAL(1, param2);

    librertos_sched();
    LONGS_EQUAL(2, param1);
    LONGS_EQUAL(2, param2);
}

TEST(Scheduler, TwoTasks_HigherPriorityHasPrecedence)
{
    librertos_create_task(HIGH_PRIORITY, &task1, &increment_param, &param1);
    librertos_create_task(LOW_PRIORITY, &task2, &increment_param, &param2);

    librertos_sched();
    LONGS_EQUAL(1, param1);
    LONGS_EQUAL(0, param2);

    librertos_sched();
    LONGS_EQUAL(2, param1);
    LONGS_EQUAL(0, param2);
}

TEST(Scheduler, InvalidPriority_CallsAssertFunction)
{
    mock()
        .expectOneCall("librertos_assert")
        .withParameter("val", LOW_PRIORITY - 1)
        .withParameter("msg", "librertos_create_task(): invalid priority.");

    CHECK_THROWS(
        AssertionError,
        librertos_create_task(
            LOW_PRIORITY - 1, &task1, &increment_param, &param1));
}

TEST(Scheduler, InvalidPriority_CallsAssertFunction_2)
{
    mock()
        .expectOneCall("librertos_assert")
        .withParameter("val", HIGH_PRIORITY + 1)
        .withParameter("msg", "librertos_create_task(): invalid priority.");

    CHECK_THROWS(
        AssertionError,
        librertos_create_task(
            HIGH_PRIORITY + 1, &task1, &increment_param, &param1));
}

TEST(Scheduler, Suspend_SuspendOneTask)
{
    librertos_create_task(HIGH_PRIORITY, &task1, &increment_param, &param1);
    librertos_create_task(LOW_PRIORITY, &task2, &increment_param, &param2);

    librertos_sched();
    LONGS_EQUAL(1, param1);
    LONGS_EQUAL(0, param2);

    task_suspend(&task1);

    librertos_sched();
    LONGS_EQUAL(1, param1);
    LONGS_EQUAL(1, param2);
}

TEST(Scheduler, Suspend_SuspendTwoTasks)
{
    librertos_create_task(HIGH_PRIORITY, &task1, &increment_param, &param1);
    librertos_create_task(LOW_PRIORITY, &task2, &increment_param, &param2);

    librertos_sched();
    LONGS_EQUAL(1, param1);
    LONGS_EQUAL(0, param2);

    task_suspend(&task1);
    task_suspend(&task2);

    librertos_sched();
    LONGS_EQUAL(1, param1);
    LONGS_EQUAL(0, param2);
}

static void increment_param_and_suspend_itself(void *param)
{
    increment_param(param);
    task_suspend(NULL);
}

TEST(Scheduler, Suspend_TaskSuspendsItselfUsingNull)
{
    librertos_create_task(
        HIGH_PRIORITY, &task1, &increment_param_and_suspend_itself, &param1);
    librertos_create_task(LOW_PRIORITY, &task2, &increment_param, &param2);

    librertos_sched();
    LONGS_EQUAL(1, param1);
    LONGS_EQUAL(0, param2);

    librertos_sched();
    LONGS_EQUAL(1, param1);
    LONGS_EQUAL(1, param2);
}

TEST(Scheduler, Resume_HigherPriority_GoesFirst)
{
    librertos_create_task(HIGH_PRIORITY, &task1, &increment_param, &param1);
    librertos_create_task(LOW_PRIORITY, &task2, &increment_param, &param2);

    task_suspend(&task1);
    task_resume(&task1);

    librertos_sched();
    LONGS_EQUAL(1, param1);
    LONGS_EQUAL(0, param2);
}

TEST(Scheduler, Resume_SamePriority_GoesToEndOfReadyList)
{
    librertos_create_task(LOW_PRIORITY, &task1, &increment_param, &param1);
    librertos_create_task(LOW_PRIORITY, &task2, &increment_param, &param2);

    task_suspend(&task1);
    task_resume(&task1);

    librertos_sched();
    LONGS_EQUAL(0, param1);
    LONGS_EQUAL(1, param2);

    librertos_sched();
    LONGS_EQUAL(1, param1);
    LONGS_EQUAL(1, param2);
}

TEST(Scheduler, Resume_SamePriority_GoesToEndOfReadyList_2)
{
    librertos_create_task(LOW_PRIORITY, &task1, &increment_param, &param1);
    librertos_create_task(LOW_PRIORITY, &task2, &increment_param, &param2);

    task_suspend(&task1);
    task_suspend(&task2);
    task_resume(&task1);
    task_resume(&task2);

    librertos_sched();
    LONGS_EQUAL(1, param1);
    LONGS_EQUAL(0, param2);

    librertos_sched();
    LONGS_EQUAL(1, param1);
    LONGS_EQUAL(1, param2);
}

TEST(Scheduler, Resume_ReadyTask_DoesNothing)
{
    librertos_create_task(LOW_PRIORITY, &task1, &increment_param, &param1);
    librertos_create_task(LOW_PRIORITY, &task2, &increment_param, &param2);

    task_resume(&task1);

    librertos_sched();
    LONGS_EQUAL(1, param1);
    LONGS_EQUAL(0, param2);

    librertos_sched();
    LONGS_EQUAL(1, param1);
    LONGS_EQUAL(1, param2);
}

TEST(Scheduler, GetCurrentTask_NoTask_ReturnNULL)
{
    POINTERS_EQUAL(NULL, get_current_task());
}

static void check_current_task(void *param)
{
    task_t **task = (task_t **)param;
    *task = get_current_task();
}

TEST(Scheduler, GetCurrentTask_RunTask_ReturnsPointer)
{
    task_t *task = NULL;

    librertos_create_task(LOW_PRIORITY, &task1, &check_current_task, &task);
    librertos_sched();

    POINTERS_EQUAL(&task1, task);
}

TEST(Scheduler, GetCurrentTask_SetCurrentTask_ReturnPointer)
{
    set_current_task(&task1);
    POINTERS_EQUAL(&task1, get_current_task());
}
