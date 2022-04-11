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

TEST_GROUP (SchedulerMode)
{
    task_t task1, task2;

    void setup() { librertos_init(); }
    void teardown() { kernel_mode = LIBRERTOS_PREEMPTIVE; }
};

static void do_nothing(void *)
{
    mock().actualCall("do_nothing");

    task_suspend(NULL);
}

static void call_scheduler(void *)
{
    mock().actualCall("call_scheduler");

    librertos_sched();
}

static void resume_task_and_call_scheduler(void *param)
{
    task_t *task_to_resume = (task_t *)param;

    mock().actualCall("resume_task_and_call_scheduler");

    if (task_to_resume != NULL)
        task_resume(task_to_resume);

    librertos_sched();
}

TEST(SchedulerMode, Preemptive_DoNotRunSamePriorityTask)
{
    kernel_mode = LIBRERTOS_PREEMPTIVE;

    librertos_create_task(HIGH_PRIORITY, &task1, &call_scheduler, NULL);
    librertos_create_task(HIGH_PRIORITY, &task2, &do_nothing, NULL);

    mock().expectOneCall("call_scheduler");
    mock().expectNoCall("do_nothing");

    librertos_sched();
}

TEST(SchedulerMode, Preemptive_OtherTaskRunning_RunHigherPriority)
{
    kernel_mode = LIBRERTOS_PREEMPTIVE;

    librertos_create_task(HIGH_PRIORITY, &task1, &do_nothing, NULL);
    librertos_create_task(
        LOW_PRIORITY, &task2, &resume_task_and_call_scheduler, &task1);

    task_suspend(&task1);

    mock().expectOneCall("do_nothing");
    mock().expectOneCall("resume_task_and_call_scheduler");

    librertos_sched();
}

TEST(SchedulerMode, Cooperative_OtherTaskRunning_DoNotRunHigherPriority)
{
    kernel_mode = LIBRERTOS_COOPERATIVE;

    librertos_create_task(HIGH_PRIORITY, &task1, &do_nothing, NULL);
    librertos_create_task(
        LOW_PRIORITY, &task2, &resume_task_and_call_scheduler, &task1);

    task_suspend(&task1);

    mock().expectNoCall("do_nothing");
    mock().expectOneCall("resume_task_and_call_scheduler");

    librertos_sched();
}
