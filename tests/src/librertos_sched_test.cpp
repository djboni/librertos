/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos.h"
#include "librertos_impl.h"

/*
 * Main file: src/librertos.c
 * Also compile: tests/mocks/librertos_assert.cpp
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

static void task_function(void *param)
{
    int *p = (int *)param;
    *p += 1;
}

TEST(Scheduler, OneTask_RunTask1)
{
    librertos_create_task(LOW_PRIORITY, &task1, &task_function, &param1);

    librertos_sched();
    LONGS_EQUAL(1, param1);

    librertos_sched();
    LONGS_EQUAL(2, param1);
}

TEST(Scheduler, TwoTasks_SamePriorityRunCooperatively)
{
    librertos_create_task(LOW_PRIORITY, &task1, &task_function, &param1);
    librertos_create_task(LOW_PRIORITY, &task2, &task_function, &param2);

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
    librertos_create_task(HIGH_PRIORITY, &task1, &task_function, &param1);
    librertos_create_task(HIGH_PRIORITY, &task2, &task_function, &param2);

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
    librertos_create_task(HIGH_PRIORITY, &task1, &task_function, &param1);
    librertos_create_task(LOW_PRIORITY, &task2, &task_function, &param2);

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
            LOW_PRIORITY - 1, &task1, &task_function, &param1));
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
            HIGH_PRIORITY + 1, &task1, &task_function, &param1));
}
