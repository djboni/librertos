/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos.h"
#include "librertos_impl.h"
#include "tests/utils/librertos_custom_tests.h"
#include "tests/utils/librertos_test_utils.h"

/*
 * Main file: src/librertos.c
 * Also compile: tests/mocks/librertos_assert.cpp
 * Also compile: tests/utils/librertos_custom_tests.cpp
 * Also compile: tests/utils/librertos_test_utils.cpp
 */

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

TEST_GROUP (Delay)
{
    void setup() { test_init(); }
    void teardown() {}
};

TEST(Delay, Delay_SuspendOneTask)
{
    auto task = test_create_tasks({0}, NULL, {NULL});

    set_current_task(task[0]);
    task_delay(1);

    POINTERS_EQUAL(librertos.tasks_delayed_current, task[0]->sched_node.list);
}

TEST(Delay, Delay_SuspendsTwoTasks)
{
    auto task = test_create_tasks({0, 0}, NULL, {NULL});

    set_current_task(task[0]);
    task_delay(1);

    set_current_task(task[1]);
    task_delay(2);

    POINTERS_EQUAL(librertos.tasks_delayed_current, task[0]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[1]->sched_node.list);
}

TEST(Delay, TickInterrupt_ResumeOneTask)
{
    auto task = test_create_tasks({0}, NULL, {NULL});

    set_current_task(task[0]);
    task_delay(1);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
}

TEST(Delay, TickInterrupt_ResumeOnlyExpiredTasks)
{
    auto task = test_create_tasks({0, 0}, NULL, {NULL});

    set_current_task(task[0]);
    task_delay(1);

    set_current_task(task[1]);
    task_delay(2);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[1]->sched_node.list);
}

TEST(Delay, TickInterrupt_ResumeOnlyExpiredTasks_2)
{
    auto task = test_create_tasks({0, 0}, NULL, {NULL});

    set_current_task(task[0]);
    task_delay(1);

    set_current_task(task[1]);
    task_delay(2);

    librertos_tick_interrupt();
    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
}

TEST(Delay, TickInterrupt_ResumeMultipleExpiredTasks)
{
    auto task = test_create_tasks({0, 0, 0}, NULL, {NULL});

    set_current_task(task[0]);
    task_delay(1);

    set_current_task(task[1]);
    task_delay(1);

    set_current_task(task[2]);
    task_delay(2);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[2]->sched_node.list);
}

TEST(Delay, TickInterrupt_TwoTasks_TimeSequence)
{
    auto task = test_create_tasks({0, 0}, NULL, {NULL});

    set_current_task(task[0]);
    task_delay(1);

    set_current_task(task[1]);
    task_delay(2);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[1]->sched_node.list);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
}

TEST(Delay, TickInterrupt_TwoTasks_TimeSequence_2)
{
    auto task = test_create_tasks({0, 0}, NULL, {NULL});

    set_current_task(task[0]);
    task_delay(2);

    set_current_task(task[1]);
    task_delay(1);

    librertos_tick_interrupt();

    POINTERS_EQUAL(librertos.tasks_delayed_current, task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
}

TEST(Delay, TickInterrupt_TwoTasks_TimeSequence_3)
{
    auto task = test_create_tasks({0, 0}, NULL, {NULL});

    set_current_task(task[0]);
    task_delay(1);

    set_current_task(task[1]);
    task_delay(1);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
}

TEST(Delay, TickInterrupt_ThreeTasks_TimeSequence)
{
    auto task = test_create_tasks({0, 0, 0}, NULL, {NULL});

    set_current_task(task[0]);
    task_delay(1);

    set_current_task(task[1]);
    task_delay(2);

    set_current_task(task[2]);
    task_delay(3);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[1]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[2]->sched_node.list);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[2]->sched_node.list);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[2]->sched_node.list);
}

TEST(Delay, TickInterrupt_ThreeTasks_TimeSequence_2)
{
    auto task = test_create_tasks({0, 0, 0}, NULL, {NULL});

    set_current_task(task[0]);
    task_delay(1);

    set_current_task(task[2]);
    task_delay(3);

    set_current_task(task[1]);
    task_delay(2);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[1]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[2]->sched_node.list);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[2]->sched_node.list);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[2]->sched_node.list);
}

TEST(Delay, TickInterrupt_ThreeTasks_TimeSequence_3)
{
    auto task = test_create_tasks({0, 0, 0}, NULL, {NULL});

    set_current_task(task[1]);
    task_delay(2);

    set_current_task(task[0]);
    task_delay(1);

    set_current_task(task[2]);
    task_delay(3);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[1]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[2]->sched_node.list);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[2]->sched_node.list);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[2]->sched_node.list);
}

TEST(Delay, TickInterrupt_ThreeTasks_TimeSequence_4)
{
    auto task = test_create_tasks({0, 0, 0}, NULL, {NULL});

    set_current_task(task[1]);
    task_delay(2);

    set_current_task(task[2]);
    task_delay(3);

    set_current_task(task[0]);
    task_delay(1);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[1]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[2]->sched_node.list);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[2]->sched_node.list);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[2]->sched_node.list);
}

TEST(Delay, TickInterrupt_ThreeTasks_TimeSequence_5)
{
    auto task = test_create_tasks({0, 0, 0}, NULL, {NULL});

    set_current_task(task[2]);
    task_delay(3);

    set_current_task(task[0]);
    task_delay(1);

    set_current_task(task[1]);
    task_delay(2);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[1]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[2]->sched_node.list);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[2]->sched_node.list);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[2]->sched_node.list);
}

TEST(Delay, TickInterrupt_ThreeTasks_TimeSequence_6)
{
    auto task = test_create_tasks({0, 0, 0}, NULL, {NULL});

    set_current_task(task[2]);
    task_delay(3);

    set_current_task(task[1]);
    task_delay(2);

    set_current_task(task[0]);
    task_delay(1);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[1]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[2]->sched_node.list);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[2]->sched_node.list);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[2]->sched_node.list);
}

TEST(Delay, TickInterrupt_ThreeTasks_TimeSequence_7)
{
    auto task = test_create_tasks({0, 0, 0}, NULL, {NULL});

    set_current_task(task[0]);
    task_delay(1);

    set_current_task(task[1]);
    task_delay(1);

    set_current_task(task[2]);
    task_delay(2);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[2]->sched_node.list);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[2]->sched_node.list);
}

TEST(Delay, TickInterrupt_ThreeTasks_TimeSequence_8)
{
    auto task = test_create_tasks({0, 0, 0}, NULL, {NULL});

    set_current_task(task[1]);
    task_delay(1);

    set_current_task(task[2]);
    task_delay(2);

    set_current_task(task[0]);
    task_delay(1);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[2]->sched_node.list);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[2]->sched_node.list);
}

TEST(Delay, TickInterrupt_ThreeTasks_TimeSequence_9)
{
    auto task = test_create_tasks({0, 0, 0}, NULL, {NULL});

    set_current_task(task[0]);
    task_delay(1);

    set_current_task(task[1]);
    task_delay(2);

    set_current_task(task[2]);
    task_delay(2);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[1]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[2]->sched_node.list);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[2]->sched_node.list);
}

TEST(Delay, TickInterrupt_ThreeTasks_TimeSequence_10)
{
    auto task = test_create_tasks({0, 0, 0}, NULL, {NULL});

    set_current_task(task[1]);
    task_delay(2);

    set_current_task(task[2]);
    task_delay(2);

    set_current_task(task[0]);
    task_delay(1);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[1]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[2]->sched_node.list);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[2]->sched_node.list);
}

TEST(Delay, TickInterrupt_ThreeTasks_TimeSequence_11)
{
    auto task = test_create_tasks({0, 0, 0}, NULL, {NULL});

    set_current_task(task[0]);
    task_delay(1);

    set_current_task(task[1]);
    task_delay(1);

    set_current_task(task[2]);
    task_delay(1);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[2]->sched_node.list);
}

TEST(Delay, Delay_SuspendOneTask_Overflowed)
{
    auto task = test_create_tasks({0}, NULL, {NULL});

    set_tick(1);

    set_current_task(task[0]);
    task_delay(MAX_DELAY);

    POINTERS_EQUAL(librertos.tasks_delayed_overflow, task[0]->sched_node.list);
}

TEST(Delay, Delay_SuspendsTwoTasks_Overflowed)
{
    auto task = test_create_tasks({0, 0}, NULL, {NULL});

    set_tick(2);

    set_current_task(task[0]);
    task_delay(MAX_DELAY - 1);

    set_current_task(task[1]);
    task_delay(MAX_DELAY);

    POINTERS_EQUAL(librertos.tasks_delayed_overflow, task[0]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_overflow, task[1]->sched_node.list);
}

TEST(Delay, TickInterrupt_Overflow_SwapDelayLists)
{
    POINTERS_EQUAL(
        &librertos.tasks_delayed[0], librertos.tasks_delayed_current);
    POINTERS_EQUAL(
        &librertos.tasks_delayed[1], librertos.tasks_delayed_overflow);

    time_travel(MAX_DELAY);
    librertos_tick_interrupt();

    POINTERS_EQUAL(
        &librertos.tasks_delayed[1], librertos.tasks_delayed_current);
    POINTERS_EQUAL(
        &librertos.tasks_delayed[0], librertos.tasks_delayed_overflow);

    time_travel(MAX_DELAY);
    librertos_tick_interrupt();

    POINTERS_EQUAL(
        &librertos.tasks_delayed[0], librertos.tasks_delayed_current);
    POINTERS_EQUAL(
        &librertos.tasks_delayed[1], librertos.tasks_delayed_overflow);
}

TEST(Delay, TickInterrupt_Overflow_ResumeTasks)
{
    auto task = test_create_tasks({0, 0, 0}, NULL, {NULL});

    set_tick(2);

    set_current_task(task[0]);
    task_delay(MAX_DELAY - 2);

    set_current_task(task[1]);
    task_delay(MAX_DELAY - 1);

    set_current_task(task[2]);
    task_delay(MAX_DELAY);

    POINTERS_EQUAL(librertos.tasks_delayed_current, task[0]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_overflow, task[1]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_overflow, task[2]->sched_node.list);

    set_tick(MAX_DELAY);
    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[2]->sched_node.list);

    librertos_tick_interrupt();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[2]->sched_node.list);
}

TEST(Delay, ResumeAllTasks_AlsoResumesDelayedTasks)
{
    auto task = test_create_tasks({0, 0, 0}, NULL, {NULL});

    set_tick(1);

    set_current_task(task[0]);
    task_suspend(NULL);

    set_current_task(task[1]);
    task_delay(MAX_DELAY - 1);

    set_current_task(task[2]);
    task_delay(MAX_DELAY);

    POINTERS_EQUAL(&librertos.tasks_suspended, task[0]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_current, task[1]->sched_node.list);
    POINTERS_EQUAL(librertos.tasks_delayed_overflow, task[2]->sched_node.list);

    task_resume_all();

    POINTERS_EQUAL(&librertos.tasks_ready[0], task[0]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[1]->sched_node.list);
    POINTERS_EQUAL(&librertos.tasks_ready[0], task[2]->sched_node.list);
}
