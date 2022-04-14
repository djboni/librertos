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

TEST_GROUP (Event)
{
    char buff[BUFF_SIZE];

    event_t event;
    task_t task1, task2;
    Param param1;

    void setup()
    {
        // Task sequencing buffer
        strcpy(buff, "");

        // Task parameters
        param1 = Param{&buff[0], "A", "B", "C", NULL, 1};

        // Initialize
        librertos_init();
        event_init(&event);
    }
    void teardown() {}
};

TEST(Event, TaskSuspendsOnEvent_ShouldNotBeScheduled)
{
    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);

    set_current_task(&task1);
    event_suspend_task(&event);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("", buff);
}

TEST(Event, TaskSuspendsOnTwoEvent_CallsAssertFunction)
{
    mock()
        .expectOneCall("librertos_assert")
        .withParameter("val", (intptr_t)&task1)
        .withParameter(
            "msg", "event_suspend_task(): this task is already suspended.");

    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);

    set_current_task(&task1);
    event_suspend_task(&event);

    CHECK_THROWS(AssertionError, event_suspend_task(&event));
}

TEST(Event, TwoTasksSuspendOnTheSameEvent_CallsAssertFunction)
{
    mock()
        .expectOneCall("librertos_assert")
        .withParameter("val", (intptr_t)&task2)
        .withParameter(
            "msg", "event_suspend_task(): another task is suspended.");

    librertos_create_task(LOW_PRIORITY, &task1, &Param::task_sequencing, NULL);
    librertos_create_task(LOW_PRIORITY, &task2, &Param::task_sequencing, NULL);

    set_current_task(&task1);
    event_suspend_task(&event);

    set_current_task(&task2);
    CHECK_THROWS(AssertionError, event_suspend_task(&event));
}

TEST(Event, CallSuspendWithNoTaskRunning_CallsAssertFunction)
{
    mock()
        .expectOneCall("librertos_assert")
        .withParameter("val", (intptr_t)NO_TASK_PTR)
        .withParameter(
            "msg", "event_suspend_task(): no task or interrupt is running.");

    CHECK_THROWS(AssertionError, event_suspend_task(&event));
}

TEST(Event, CallSuspendWithInterruptRunning_CallsAssertFunction)
{
    mock()
        .expectOneCall("librertos_assert")
        .withParameter("val", (intptr_t)INTERRUPT_TASK_PTR)
        .withParameter(
            "msg", "event_suspend_task(): no task or interrupt is running.");

    (void)interrupt_lock();

    CHECK_THROWS(AssertionError, event_suspend_task(&event));
}

TEST(Event, TaskResumedOnEvent_ShouldBeScheduled)
{
    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);

    set_current_task(&task1);
    event_suspend_task(&event);
    event_resume_task(&event);
    set_current_task(NO_TASK_PTR);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("ABC", buff);
}

TEST(Event, NoTaskToResume_OK)
{
    event_resume_task(&event);
}
