/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos.h"
#include "librertos_impl.h"
#include "tests/utils/librertos_test_utils.h"

#include <cstring>

/*
 * Main file: src/librertos.c
 * Also compile: tests/mocks/librertos_assert.cpp
 * Also compile: tests/utils/librertos_test_utils.cpp
 */

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#define BUFF_SIZE 128

struct Param
{
    char *buff;
    const char *start, *resume, *sched, *end;
    task_t *task_to_resume;
    int suspend_after_n_runs;
};

static void task_sequencing(void *param)
{
    Param *p = (Param *)param;

    if (p == NULL)
        return;

    // Concat start
    strcat(p->buff, p->start);

    // Resume task
    if (p->task_to_resume != NULL)
        task_resume(p->task_to_resume);

    // Concat resume
    strcat(p->buff, p->resume);

    // Run scheduler
    librertos_sched();

    // Concat scheduler
    strcat(p->buff, p->sched);

    // Suspend itself
    if (--(p->suspend_after_n_runs) <= 0)
        task_suspend(NULL);

    // Concat end
    strcat(p->buff, p->end);
}

TEST_GROUP (Scheduler)
{
    task_t task1, task2;

    void setup() { librertos_init(); }
    void teardown() {}
};

TEST(Scheduler, NoTask_DoNothing)
{
    librertos_sched();
}

TEST(Scheduler, OneTask_RunTask)
{
    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", "D", NULL, 2};

    librertos_create_task(LOW_PRIORITY, &task1, &task_sequencing, &param1);

    librertos_sched();
    STRCMP_EQUAL("ABCDABCD", buff);
}

TEST(Scheduler, TwoTasks_SamePriorityRunCooperatively)
{
    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", "D", NULL, 2};
    Param param2 = {&buff[0], "e", "f", "g", "h", NULL, 2};

    librertos_create_task(LOW_PRIORITY, &task1, &task_sequencing, &param1);
    librertos_create_task(LOW_PRIORITY, &task2, &task_sequencing, &param2);

    librertos_sched();
    STRCMP_EQUAL("ABCDefghABCDefgh", buff);
}

TEST(Scheduler, TwoTasks_SamePriorityRunCooperatively_2)
{
    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", "D", NULL, 2};
    Param param2 = {&buff[0], "e", "f", "g", "h", NULL, 2};

    librertos_create_task(HIGH_PRIORITY, &task1, &task_sequencing, &param1);
    librertos_create_task(HIGH_PRIORITY, &task2, &task_sequencing, &param2);

    librertos_sched();
    STRCMP_EQUAL("ABCDefghABCDefgh", buff);
}

TEST(Scheduler, TwoTasks_HigherPriorityHasPrecedence)
{
    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", "D", NULL, 2};
    Param param2 = {&buff[0], "e", "f", "g", "h", NULL, 2};

    librertos_create_task(HIGH_PRIORITY, &task1, &task_sequencing, &param1);
    librertos_create_task(LOW_PRIORITY, &task2, &task_sequencing, &param2);

    librertos_sched();
    STRCMP_EQUAL("ABCDABCDefghefgh", buff);
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
            LOW_PRIORITY - 1, &task1, &task_sequencing, NULL));
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
            HIGH_PRIORITY + 1, &task1, &task_sequencing, NULL));
}

TEST(Scheduler, Suspend_SuspendOneTask)
{
    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", "D", NULL, 2};
    Param param2 = {&buff[0], "e", "f", "g", "h", NULL, 2};

    librertos_create_task(HIGH_PRIORITY, &task1, &task_sequencing, &param1);
    librertos_create_task(LOW_PRIORITY, &task2, &task_sequencing, &param2);

    task_suspend(&task1);

    librertos_sched();
    STRCMP_EQUAL("efghefgh", buff);
}

TEST(Scheduler, Suspend_SuspendTwoTasks)
{
    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", "D", NULL, 2};
    Param param2 = {&buff[0], "e", "f", "g", "h", NULL, 2};

    librertos_create_task(HIGH_PRIORITY, &task1, &task_sequencing, &param1);
    librertos_create_task(LOW_PRIORITY, &task2, &task_sequencing, &param2);

    task_suspend(&task1);
    task_suspend(&task2);

    librertos_sched();
    STRCMP_EQUAL("", buff);
}

TEST(Scheduler, Suspend_TaskSuspendsItselfUsingNull)
{
    /* Dummy test. Nothing to test, because task_sequencing() already uses the
     * feature of a task suspending itself calling task_suspend(NULL).
     */
    volatile task_function_t func = &task_sequencing;
    CHECK_TRUE(func != NULL);
}

TEST(Scheduler, Resume_HigherPriority_GoesFirst)
{
    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", "D", NULL, 1};
    Param param2 = {&buff[0], "e", "f", "g", "h", NULL, 1};

    librertos_create_task(HIGH_PRIORITY, &task1, &task_sequencing, &param1);
    librertos_create_task(LOW_PRIORITY, &task2, &task_sequencing, &param2);

    task_suspend(&task1);
    task_resume(&task1);

    librertos_sched();
    STRCMP_EQUAL("ABCDefgh", buff);
}

TEST(Scheduler, Resume_SamePriority_GoesToEndOfReadyList)
{
    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", "D", NULL, 1};
    Param param2 = {&buff[0], "e", "f", "g", "h", NULL, 1};

    librertos_create_task(LOW_PRIORITY, &task1, &task_sequencing, &param1);
    librertos_create_task(LOW_PRIORITY, &task2, &task_sequencing, &param2);

    task_suspend(&task1);
    task_resume(&task1);

    librertos_sched();
    STRCMP_EQUAL("efghABCD", buff);
}

TEST(Scheduler, Resume_SamePriority_GoesToEndOfReadyList_2)
{
    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", "D", NULL, 1};
    Param param2 = {&buff[0], "e", "f", "g", "h", NULL, 1};

    librertos_create_task(LOW_PRIORITY, &task1, &task_sequencing, &param1);
    librertos_create_task(LOW_PRIORITY, &task2, &task_sequencing, &param2);

    task_suspend(&task1);
    task_suspend(&task2);
    task_resume(&task1);
    task_resume(&task2);

    librertos_sched();
    STRCMP_EQUAL("ABCDefgh", buff);
}

TEST(Scheduler, Resume_ReadyTask_DoesNothing)
{
    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", "D", NULL, 1};
    Param param2 = {&buff[0], "e", "f", "g", "h", NULL, 1};

    librertos_create_task(LOW_PRIORITY, &task1, &task_sequencing, &param1);
    librertos_create_task(LOW_PRIORITY, &task2, &task_sequencing, &param2);

    task_resume(&task1);

    librertos_sched();
    STRCMP_EQUAL("ABCDefgh", buff);
}

TEST(Scheduler, GetCurrentTask_NoTask_ReturnNULL)
{
    POINTERS_EQUAL(NULL, get_current_task());
}

static void check_current_task(void *param)
{
    task_t **task = (task_t **)param;
    *task = get_current_task();
    task_suspend(NULL);
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

TEST_GROUP (SchedulerMode)
{
    task_t task1, task2, task3;

    void setup() { librertos_init(); }
    void teardown() { kernel_mode = LIBRERTOS_PREEMPTIVE; }
};

TEST(SchedulerMode, Cooperative_OtherTaskRunning_DoNotRunHigherPriority)
{
    /*
     * - Task A runs
     *   - Resumes task B (higher priority)
     *   - Task A finishes
     * - Task B runs and finishes
     */

    kernel_mode = LIBRERTOS_COOPERATIVE;

    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", "D", &task2, 1};
    Param param2 = {&buff[0], "e", "f", "g", "h", NULL, 1};

    librertos_create_task(LOW_PRIORITY, &task1, &task_sequencing, &param1);
    librertos_create_task(HIGH_PRIORITY, &task2, &task_sequencing, &param2);

    task_suspend(&task2);

    librertos_sched();

    STRCMP_EQUAL("ABCDefgh", buff);
}

TEST(SchedulerMode, Preemptive_DoNotRunSamePriorityTask)
{
    /*
     * - Task A runs
     *   - Resumes task B (same priority)
     *   - Task A finishes
     * - Task B runs and finishes
     */

    kernel_mode = LIBRERTOS_PREEMPTIVE;

    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", "D", &task2, 1};
    Param param2 = {&buff[0], "e", "f", "g", "h", NULL, 1};

    librertos_create_task(LOW_PRIORITY, &task1, &task_sequencing, &param1);
    librertos_create_task(LOW_PRIORITY, &task2, &task_sequencing, &param2);

    task_suspend(&task2);

    librertos_sched();

    STRCMP_EQUAL("ABCDefgh", buff);
}

TEST(SchedulerMode, Preemptive_OtherTaskRunning_RunHigherPriority)
{
    /*
     * - Task A (lower priority) runs
     *   - Resumes task B (higher priority) and gets preempted
     * - Task B runs and finishes
     * - Task A finishes
     */

    kernel_mode = LIBRERTOS_PREEMPTIVE;

    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", "D", &task2, 1};
    Param param2 = {&buff[0], "e", "f", "g", "h", NULL, 1};

    librertos_create_task(LOW_PRIORITY, &task1, &task_sequencing, &param1);
    librertos_create_task(HIGH_PRIORITY, &task2, &task_sequencing, &param2);

    task_suspend(&task2);

    librertos_sched();

    STRCMP_EQUAL("ABefghCD", buff);
}

TEST(SchedulerMode, Preemptive_ResumedHighPrioTask_KeepsLowPrioPreempted)
{
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
    Param param1 = {&buff[0], "A", "B", "C", "D", &task2, 1};
    Param param2 = {&buff[0], "e", "f", "g", "h", &task3, 1};
    Param param3 = {&buff[0], "1", "2", "3", "4", NULL, 1};

    librertos_create_task(LOW_PRIORITY, &task1, &task_sequencing, &param1);
    librertos_create_task(HIGH_PRIORITY, &task2, &task_sequencing, &param2);
    librertos_create_task(HIGH_PRIORITY, &task3, &task_sequencing, &param3);

    task_suspend(&task2);
    task_suspend(&task3);

    librertos_sched();

    STRCMP_EQUAL("ABefgh1234CD", buff);
}
