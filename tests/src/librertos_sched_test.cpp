/* Copyright (c) 2016-2023 Djones A. Boni - MIT License */

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

TEST_GROUP (Scheduler) {
    task_t task1, task2;

    task_t task[2];

    void setup() {
        librertos_init();
    }
    void teardown() {
    }
};

TEST(Scheduler, NoTask_DoNothing) {
    librertos_start();
    librertos_sched();
}

TEST(Scheduler, OneTask_RunTask) {
    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", NULL, 2};

    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("ABCABC", buff);
}

TEST(Scheduler, TwoTasks_SamePriorityRunCooperatively) {
    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", NULL, 2};
    Param param2 = {&buff[0], "e", "f", "g", NULL, 2};

    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);
    librertos_create_task(
        LOW_PRIORITY, &task2, &Param::task_sequencing, &param2);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("ABCefgABCefg", buff);
}

TEST(Scheduler, TwoTasks_SamePriorityRunCooperatively_2) {
    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", NULL, 2};
    Param param2 = {&buff[0], "e", "f", "g", NULL, 2};

    librertos_create_task(
        HIGH_PRIORITY, &task1, &Param::task_sequencing, &param1);
    librertos_create_task(
        HIGH_PRIORITY, &task2, &Param::task_sequencing, &param2);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("ABCefgABCefg", buff);
}

TEST(Scheduler, TwoTasks_HigherPriorityHasPrecedence) {
    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", NULL, 2};
    Param param2 = {&buff[0], "e", "f", "g", NULL, 2};

    librertos_create_task(
        HIGH_PRIORITY, &task1, &Param::task_sequencing, &param1);
    librertos_create_task(
        LOW_PRIORITY, &task2, &Param::task_sequencing, &param2);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("ABCABCefgefg", buff);
}

TEST(Scheduler, InvalidPriority_CallsAssertFunction) {
    mock()
        .expectOneCall("librertos_assert")
        .withParameter("val", LOW_PRIORITY - 1)
        .withParameter("msg", "librertos_create_task(): invalid priority.");

    CHECK_THROWS(
        AssertionError,
        librertos_create_task(
            LOW_PRIORITY - 1, &task1, &Param::task_sequencing, NULL));
}

TEST(Scheduler, InvalidPriority_CallsAssertFunction_2) {
    mock()
        .expectOneCall("librertos_assert")
        .withParameter("val", HIGH_PRIORITY + 1)
        .withParameter("msg", "librertos_create_task(): invalid priority.");

    CHECK_THROWS(
        AssertionError,
        librertos_create_task(
            HIGH_PRIORITY + 1, &task1, &Param::task_sequencing, NULL));
}

TEST(Scheduler, Suspend_SuspendOneTask) {
    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", NULL, 2};
    Param param2 = {&buff[0], "e", "f", "g", NULL, 2};

    librertos_create_task(
        HIGH_PRIORITY, &task1, &Param::task_sequencing, &param1);
    librertos_create_task(
        LOW_PRIORITY, &task2, &Param::task_sequencing, &param2);

    task_suspend(&task1);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("efgefg", buff);
}

TEST(Scheduler, Suspend_SuspendTwoTasks) {
    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", NULL, 2};
    Param param2 = {&buff[0], "e", "f", "g", NULL, 2};

    librertos_create_task(
        HIGH_PRIORITY, &task1, &Param::task_sequencing, &param1);
    librertos_create_task(
        LOW_PRIORITY, &task2, &Param::task_sequencing, &param2);

    task_suspend(&task1);
    task_suspend(&task2);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("", buff);
}

TEST(Scheduler, Suspend_TaskSuspendsItselfUsingNullAkaCurrentTaskPtr) {
    /* Dummy test. Nothing to test, because Param::task_sequencing() already
     * uses the feature which a task suspends itself by calling
     * task_suspend(CURRENT_TASK_PTR).
     */
    volatile task_function_t func = &Param::task_sequencing;
    CHECK_TRUE(func != NULL);
}

TEST(Scheduler, Suspend_CallSuspendWithNoTaskRunning_CallsAssertFunction) {
    mock()
        .expectOneCall("librertos_assert")
        .withParameter("val", (intptr_t)NO_TASK_PTR)
        .withParameter(
            "msg", "task_suspend(): no task or interrupt is running.");

    CHECK_THROWS(AssertionError, task_suspend(CURRENT_TASK_PTR));
}

TEST(Scheduler, Suspend_CallSuspendWithInterruptRunning_CallsAssertFunction) {
    mock()
        .expectOneCall("librertos_assert")
        .withParameter("val", (intptr_t)INTERRUPT_TASK_PTR)
        .withParameter(
            "msg", "task_suspend(): no task or interrupt is running.");

    (void)interrupt_lock();

    CHECK_THROWS(AssertionError, task_suspend(CURRENT_TASK_PTR));
}

TEST(Scheduler, Resume_HigherPriority_GoesFirst) {
    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", NULL, 1};
    Param param2 = {&buff[0], "e", "f", "g", NULL, 1};

    librertos_create_task(
        HIGH_PRIORITY, &task1, &Param::task_sequencing, &param1);
    librertos_create_task(
        LOW_PRIORITY, &task2, &Param::task_sequencing, &param2);

    task_suspend(&task1);
    task_resume(&task1);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("ABCefg", buff);
}

TEST(Scheduler, Resume_SamePriority_GoesToEndOfReadyList) {
    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", NULL, 1};
    Param param2 = {&buff[0], "e", "f", "g", NULL, 1};

    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);
    librertos_create_task(
        LOW_PRIORITY, &task2, &Param::task_sequencing, &param2);

    task_suspend(&task1);
    task_resume(&task1);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("efgABC", buff);
}

TEST(Scheduler, Resume_SamePriority_GoesToEndOfReadyList_2) {
    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", NULL, 1};
    Param param2 = {&buff[0], "e", "f", "g", NULL, 1};

    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);
    librertos_create_task(
        LOW_PRIORITY, &task2, &Param::task_sequencing, &param2);

    task_suspend(&task1);
    task_suspend(&task2);
    task_resume(&task1);
    task_resume(&task2);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("ABCefg", buff);
}

TEST(Scheduler, ResumeAll_OneSuspendedTask) {
    librertos_init();

    librertos_create_task(LOW_PRIORITY, &task[0], NULL, NULL);

    task_suspend(&task[0]);

    LONGS_EQUAL(1, librertos.tasks_suspended.length);

    task_resume_all();

    LONGS_EQUAL(0, librertos.tasks_suspended.length);
}

TEST(Scheduler, ResumeAll_TwoSuspendedTasks) {
    librertos_init();

    librertos_create_task(LOW_PRIORITY, &task[0], NULL, NULL);
    librertos_create_task(LOW_PRIORITY, &task[1], NULL, NULL);

    task_suspend(&task[0]);
    task_suspend(&task[1]);

    LONGS_EQUAL(2, librertos.tasks_suspended.length);

    task_resume_all();

    LONGS_EQUAL(0, librertos.tasks_suspended.length);
}

TEST(Scheduler, ResumeAll_NoSuspendedTasks) {
    librertos_init();

    LONGS_EQUAL(0, librertos.tasks_suspended.length);

    task_resume_all();

    LONGS_EQUAL(0, librertos.tasks_suspended.length);
}

TEST(Scheduler, GetCurrentTask_NoTask_ReturnNullAkaNoTaskPtr) {
    POINTERS_EQUAL(NO_TASK_PTR, get_current_task());
}

static void check_current_task(void *param) {
    task_t **task = (task_t **)param;
    *task = get_current_task();
    task_suspend(CURRENT_TASK_PTR);
}

TEST(Scheduler, GetCurrentTask_RunTask_ReturnsPointer) {
    task_t *task = NO_TASK_PTR;

    librertos_create_task(LOW_PRIORITY, &task1, &check_current_task, &task);
    librertos_start();
    librertos_sched();

    POINTERS_EQUAL(&task1, task);
}

TEST(Scheduler, GetCurrentTask_SetCurrentTask_ReturnPointer) {
    set_current_task(&task1);
    POINTERS_EQUAL(&task1, get_current_task());
}

TEST_GROUP (SchedulerMode) {
    task_t task1, task2, task3;

    void setup() {
        librertos_init();
    }
    void teardown() {
        kernel_mode = LIBRERTOS_PREEMPTIVE;
    }
};

TEST(SchedulerMode, Cooperative_OtherTaskRunning_DoNotRunHigherPriority) {
    /*
     * - Task A runs
     *   - Resumes task B (higher priority)
     *   - Task A finishes
     * - Task B runs and finishes
     */

    kernel_mode = LIBRERTOS_COOPERATIVE;

    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", &task2, 1};
    Param param2 = {&buff[0], "e", "f", "g", NULL, 1};

    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);
    librertos_create_task(
        HIGH_PRIORITY, &task2, &Param::task_sequencing, &param2);

    task_suspend(&task2);

    librertos_start();
    librertos_sched();

    STRCMP_EQUAL("ABCefg", buff);
}

TEST(SchedulerMode, Cooperative_DoNotPreemptWithSchedulerLocked) {
    /*
     * - Task A (lower priority) runs
     *   - Resumes task B (higher priority) and finishes
     *   - Task A does not get preempted because of the cooperative kernel
     * - Task B runs and finishes
     */

    kernel_mode = LIBRERTOS_COOPERATIVE;

    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", &task2, 1};
    Param param2 = {&buff[0], "e", "f", "g", NULL, 1};

    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);
    librertos_create_task(
        HIGH_PRIORITY, &task2, &Param::task_sequencing, &param2);

    task_suspend(&task2);

    librertos_start();
    scheduler_lock();
    librertos_sched();
    scheduler_unlock();

    STRCMP_EQUAL("ABCefg", buff);
}

TEST(SchedulerMode, Preemptive_DoNotRunSamePriorityTask) {
    /*
     * - Task A runs
     *   - Resumes task B (same priority)
     *   - Task A finishes
     * - Task B runs and finishes
     */

    kernel_mode = LIBRERTOS_PREEMPTIVE;

    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", &task2, 1};
    Param param2 = {&buff[0], "e", "f", "g", NULL, 1};

    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);
    librertos_create_task(
        LOW_PRIORITY, &task2, &Param::task_sequencing, &param2);

    task_suspend(&task2);

    librertos_start();
    librertos_sched();

    STRCMP_EQUAL("ABCefg", buff);
}

TEST(SchedulerMode, Preemptive_OtherTaskRunning_RunHigherPriority) {
    /*
     * - Task A (lower priority) runs
     *   - Resumes task B (higher priority) and gets preempted
     * - Task B runs and finishes
     * - Task A finishes
     */

    kernel_mode = LIBRERTOS_PREEMPTIVE;

    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", &task2, 1};
    Param param2 = {&buff[0], "e", "f", "g", NULL, 1};

    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);
    librertos_create_task(
        HIGH_PRIORITY, &task2, &Param::task_sequencing, &param2);

    task_suspend(&task2);

    librertos_start();
    librertos_sched();

    STRCMP_EQUAL("AefgBC", buff);
}

TEST(SchedulerMode, Preemptive_ResumedHighPrioTask_KeepsLowPrioPreempted) {
    /*
     * - Task A (lower priority) runs
     *   - Resumes task B (higher priority) and gets preempted
     * - Task B runs
     *   - Resumes task C (same priority) and finishes
     * - Task C runs and finishes
     * - Task A finishes
     */

    kernel_mode = LIBRERTOS_PREEMPTIVE;

    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", &task2, 1};
    Param param2 = {&buff[0], "e", "f", "g", &task3, 1};
    Param param3 = {&buff[0], "1", "2", "3", NULL, 1};

    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);
    librertos_create_task(
        HIGH_PRIORITY, &task2, &Param::task_sequencing, &param2);
    librertos_create_task(
        HIGH_PRIORITY, &task3, &Param::task_sequencing, &param3);

    task_suspend(&task2);
    task_suspend(&task3);

    librertos_start();
    librertos_sched();

    STRCMP_EQUAL("Aefg123BC", buff);
}

TEST(SchedulerMode, Preemptive_DoNotPreemptWithSchedulerLocked) {
    /*
     * - Task A (lower priority) runs
     *   - Resumes task B (higher priority) and finishes
     *   - Task A does not get preempted because of the locked scheduler
     * - Task B runs and finishes
     */

    kernel_mode = LIBRERTOS_PREEMPTIVE;

    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", &task2, 1};
    Param param2 = {&buff[0], "e", "f", "g", NULL, 1};

    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing_lock_scheduler, &param1);
    librertos_create_task(
        HIGH_PRIORITY, &task2, &Param::task_sequencing, &param2);

    task_suspend(&task2);

    librertos_start();
    librertos_sched();

    STRCMP_EQUAL("ABCefg", buff);
}

TEST(SchedulerMode, Preemptive_SchedulerLock_DoesNotChangeCurrentTask) {
    kernel_mode = LIBRERTOS_PREEMPTIVE;

    librertos_create_task(LOW_PRIORITY, &task1, &Param::task_sequencing, NULL);

    set_current_task(&task1);

    POINTERS_EQUAL(&task1, get_current_task());
    scheduler_lock();
    POINTERS_EQUAL(&task1, get_current_task());
    scheduler_unlock();
    POINTERS_EQUAL(&task1, get_current_task());
}

TEST(SchedulerMode, Cooperative_SchedulerLock_DoesNotChangeCurrentTask) {
    kernel_mode = LIBRERTOS_COOPERATIVE;

    librertos_create_task(LOW_PRIORITY, &task1, &Param::task_sequencing, NULL);

    set_current_task(&task1);

    POINTERS_EQUAL(&task1, get_current_task());
    scheduler_lock();
    POINTERS_EQUAL(&task1, get_current_task());
    scheduler_unlock();
    POINTERS_EQUAL(&task1, get_current_task());
}

TEST(
    SchedulerMode,
    Preemptive_InterruptLock_ChangesCurrentTaskToInterruptTaskPtr) {
    kernel_mode = LIBRERTOS_PREEMPTIVE;

    librertos_create_task(LOW_PRIORITY, &task1, &Param::task_sequencing, NULL);

    set_current_task(&task1);

    POINTERS_EQUAL(&task1, get_current_task());
    task_t *task = interrupt_lock();
    POINTERS_EQUAL(INTERRUPT_TASK_PTR, get_current_task());
    interrupt_unlock(task);
    POINTERS_EQUAL(&task1, get_current_task());
}

TEST(
    SchedulerMode,
    Cooperative_InterruptLock_ChangesCurrentTaskToInterruptTaskPtr) {
    kernel_mode = LIBRERTOS_COOPERATIVE;

    librertos_create_task(LOW_PRIORITY, &task1, &Param::task_sequencing, NULL);

    set_current_task(&task1);

    POINTERS_EQUAL(&task1, get_current_task());
    task_t *task = interrupt_lock();
    POINTERS_EQUAL(INTERRUPT_TASK_PTR, get_current_task());
    interrupt_unlock(task);
    POINTERS_EQUAL(&task1, get_current_task());
}

TEST_GROUP (SchedulerStart) {
    task_t task1, task2, task3;

    void setup() {
        librertos_init();
    }
    void teardown() {
        kernel_mode = LIBRERTOS_PREEMPTIVE;
    }
};

TEST(SchedulerStart, Preemptive_DoNotScheduleBeforeFinishingInitialization) {
    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", NULL, 1};

    kernel_mode = LIBRERTOS_PREEMPTIVE;
    librertos_init();
    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);

    // There might be some interrupts here while initializing the peripherals
    interrupt_unlock(interrupt_lock());

    STRCMP_EQUAL("", buff);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("ABC", buff);
}

TEST(SchedulerStart, Preemptive_IfStarted_ScheduleWhenCreatingTask) {
    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", NULL, 1};

    kernel_mode = LIBRERTOS_PREEMPTIVE;
    librertos_init();

    librertos_start();

    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);

    STRCMP_EQUAL("ABC", buff);

    librertos_sched();
    STRCMP_EQUAL("ABC", buff);
}

TEST(SchedulerStart, Preemptive_IfNotStarted_ScheulerCallsAssertFunction) {
    kernel_mode = LIBRERTOS_PREEMPTIVE;

    mock()
        .expectOneCall("librertos_assert")
        .withParameter("val", 1)
        .withParameter(
            "msg",
            "librertos_sched(): must call librertos_start() before the "
            "scheduler.");

    CHECK_THROWS(AssertionError, librertos_sched());
}

TEST(SchedulerStart, Cooperative_IfNotStarted_ScheulerCallsAssertFunction) {
    kernel_mode = LIBRERTOS_COOPERATIVE;

    mock()
        .expectOneCall("librertos_assert")
        .withParameter("val", 1)
        .withParameter(
            "msg",
            "librertos_sched(): must call librertos_start() before the "
            "scheduler.");

    CHECK_THROWS(AssertionError, librertos_sched());
}
