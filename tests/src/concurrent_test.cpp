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

static struct
{
    task_t task[2];
    semaphore_t sem;
} testx;

TEST_GROUP (Concurrent) {
    void setup() {
        librertos_init();
    }
    void teardown() {
    }
};

static void func_test1_task0(void *) {
    // Could not lock, suspend task.
    LONGS_EQUAL(0, semaphore_lock_suspend(&testx.sem, MAX_DELAY));
}

static void func_test1_task1(void *) {
    // Could not lock, suspend task.
    LONGS_EQUAL(0, semaphore_lock_suspend(&testx.sem, MAX_DELAY));
}

static void func_test1_interrupt_resume_suspend_task0() {
    task_resume(&testx.task[0]);
    task_suspend(&testx.task[0]);
}

TEST(
    Concurrent,
    Event_PosTaskRemovedFromEventWhileSuspending_LowerOrEqualPriority) {
    auto func = &func_test1_interrupt_resume_suspend_task0;
    (void)func;

    librertos_create_task(
        LOW_PRIORITY, &testx.task[0], &func_test1_task0, NULL);
    librertos_create_task(
        LOW_PRIORITY, &testx.task[1], &func_test1_task1, NULL);
    semaphore_init_locked(&testx.sem, 1);

    test_task_is_ready(&testx.task[0]);
    test_task_is_ready(&testx.task[1]);

    librertos_start();
    librertos_sched();

    test_task_is_suspended(&testx.task[0]);
    test_task_is_suspended(&testx.task[1]);
}

static void func_test2_task0(void *) {
    // Could not lock, suspend task.
    LONGS_EQUAL(0, semaphore_lock_suspend(&testx.sem, MAX_DELAY));

    // Resume Task1
    task_resume(&testx.task[1]);
}

static void func_test2_task1(void *) {
    // Could not lock, suspend task.
    LONGS_EQUAL(0, semaphore_lock_suspend(&testx.sem, MAX_DELAY));
}

static void func_test2_interrupt_resume_suspend_task0() {
    task_resume(&testx.task[0]);
    task_suspend(&testx.task[0]);
}

TEST(Concurrent, Event_PosTaskRemovedFromEventWhileSuspending_HigherPriority) {
    auto func = &func_test2_interrupt_resume_suspend_task0;
    (void)func;

    librertos_create_task(
        LOW_PRIORITY, &testx.task[0], &func_test2_task0, NULL);
    librertos_create_task(
        HIGH_PRIORITY, &testx.task[1], &func_test2_task1, NULL);
    semaphore_init_locked(&testx.sem, 1);

    task_suspend(&testx.task[1]);

    test_task_is_ready(&testx.task[0]);
    test_task_is_suspended(&testx.task[1]);

    librertos_start();
    librertos_sched();

    test_task_is_suspended(&testx.task[0]);
    test_task_is_suspended(&testx.task[1]);
}

static void func_test3_task0(void *) {
    // Could not lock, suspend task.
    LONGS_EQUAL(0, semaphore_lock_suspend(&testx.sem, MAX_DELAY));

    // Resume Task1
    task_resume(&testx.task[1]);
}

static void func_test3_task1(void *) {
    // Could not lock, suspend task.
    LONGS_EQUAL(0, semaphore_lock_suspend(&testx.sem, MAX_DELAY));
}

static void func_test3_interrupt_resume_suspend_task0() {
    semaphore_t sem2;

    semaphore_init_locked(&sem2, 1);

    task_resume(&testx.task[0]);

    task_t *current_task = get_current_task();
    set_current_task(&testx.task[0]);
    // Could not lock, suspend task.
    LONGS_EQUAL(0, semaphore_lock_suspend(&sem2, MAX_DELAY));
    set_current_task(current_task);
}

TEST(Concurrent, Event_PosTaskRemovedFromEventWhileSuspending_HigherPriority_2) {
    auto func = &func_test3_interrupt_resume_suspend_task0;
    (void)func;

    librertos_create_task(
        LOW_PRIORITY, &testx.task[0], &func_test3_task0, NULL);
    librertos_create_task(
        HIGH_PRIORITY, &testx.task[1], &func_test3_task1, NULL);
    semaphore_init_locked(&testx.sem, 1);

    task_suspend(&testx.task[1]);

    test_task_is_ready(&testx.task[0]);
    test_task_is_suspended(&testx.task[1]);

    librertos_start();
    librertos_sched();

    test_task_is_suspended(&testx.task[0]);
    test_task_is_suspended(&testx.task[1]);
}

static void func_test4_task0(void *) {
    // Could not lock, suspend task.
    LONGS_EQUAL(0, semaphore_lock_suspend(&testx.sem, MAX_DELAY));
}

static void func_test4_interrupt_resume_task0() {
    task_resume(&testx.task[0]);
}

TEST(Concurrent, Event_TaskResumedWhileSuspending) {
    auto func = &func_test4_interrupt_resume_task0;
    (void)func;

    librertos_create_task(
        LOW_PRIORITY, &testx.task[0], &func_test4_task0, NULL);
    semaphore_init_locked(&testx.sem, 1);

    test_task_is_ready(&testx.task[0]);

    librertos_start();
    librertos_sched();

    test_task_is_suspended(&testx.task[0]);
}

static void func_test5_task0(void *) {
    task_delay(10);
}

static void func_test5_task1(void *) {
    task_delay(10);
}

static void func_test5_interrupt_resume_suspend_task0() {
    task_resume(&testx.task[0]);
}

TEST(Concurrent, Delay_PosTaskRemoved_GreaterOrEqualTick) {
    auto func = &func_test5_interrupt_resume_suspend_task0;
    (void)func;

    librertos_create_task(
        LOW_PRIORITY, &testx.task[0], &func_test5_task0, NULL);
    librertos_create_task(
        LOW_PRIORITY, &testx.task[1], &func_test5_task1, NULL);

    test_task_is_ready(&testx.task[0]);
    test_task_is_ready(&testx.task[1]);

    librertos_start();
    librertos_sched();

    test_task_is_delayed_current(&testx.task[0]);
    test_task_is_delayed_current(&testx.task[1]);
}

static void func_test6_task0(void *) {
    task_delay(10);
}

static void func_test6_task1(void *) {
    task_delay(9);
}

static void func_test6_interrupt_resume_suspend_task0() {
    task_resume(&testx.task[0]);
}

TEST(Concurrent, Delay_PosTaskRemoved_LowerTick) {
    auto func = &func_test6_interrupt_resume_suspend_task0;
    (void)func;

    librertos_create_task(
        LOW_PRIORITY, &testx.task[0], &func_test6_task0, NULL);
    librertos_create_task(
        LOW_PRIORITY, &testx.task[1], &func_test6_task1, NULL);

    test_task_is_ready(&testx.task[0]);
    test_task_is_ready(&testx.task[1]);

    librertos_start();
    librertos_sched();

    test_task_is_delayed_current(&testx.task[0]);
    test_task_is_delayed_current(&testx.task[1]);
}

static void func_test7_task0(void *) {
    task_delay(10);
}

static void func_test7_task1(void *) {
    task_delay(9);
}

static void func_test7_interrupt_resume_task1() {
    task_resume(&testx.task[1]);
}

TEST(Concurrent, Delay_TaskResumedWhileSuspending) {
    auto func = &func_test7_interrupt_resume_task1;
    (void)func;

    librertos_create_task(
        LOW_PRIORITY, &testx.task[0], &func_test7_task0, NULL);
    librertos_create_task(
        LOW_PRIORITY, &testx.task[1], &func_test7_task1, NULL);

    test_task_is_ready(&testx.task[0]);
    test_task_is_ready(&testx.task[1]);

    librertos_start();
    librertos_sched();

    test_task_is_delayed_current(&testx.task[0]);
    test_task_is_delayed_current(&testx.task[1]);
}
