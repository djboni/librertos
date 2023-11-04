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

TEST_GROUP (Event) {
    char buff[BUFF_SIZE];

    event_t event;
    task_t task1, task2, task3;
    Param param1, param2, param3;

    void setup() {
        // Task sequencing buffer
        strcpy(buff, "");

        // Task parameters
        param1 = Param{&buff[0], "A", "B", "C", NULL, 1};
        param2 = Param{&buff[0], "a", "b", "c", NULL, 1};
        param3 = Param{&buff[0], "1", "2", "3", NULL, 1};

        // Initialize
        librertos_init();
        event_init(&event);
    }
    void teardown() {
    }
};

TEST(Event, TaskSuspendsOnEvent_ShouldNotBeScheduled) {
    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);

    set_current_task(&task1);
    event_delay_task(&event, MAX_DELAY);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("", buff);
}

TEST(Event, TaskSuspendsOnTwoEvent_CallsAssertFunction) {
    mock()
        .expectOneCall("librertos_assert")
        .withParameter("val", (intptr_t)&task1)
        .withParameter(
            "msg", "event_delay_task(): this task is already suspended.");

    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);

    set_current_task(&task1);
    event_delay_task(&event, MAX_DELAY);

    CHECK_THROWS(AssertionError, event_delay_task(&event, MAX_DELAY));
}

TEST(Event, CallSuspendWithNoTaskRunning_CallsAssertFunction) {
    mock()
        .expectOneCall("librertos_assert")
        .withParameter("val", (intptr_t)NO_TASK_PTR)
        .withParameter(
            "msg", "event_delay_task(): no task or interrupt is running.");

    CHECK_THROWS(AssertionError, event_delay_task(&event, MAX_DELAY));
}

TEST(Event, CallSuspendWithInterruptRunning_CallsAssertFunction) {
    mock()
        .expectOneCall("librertos_assert")
        .withParameter("val", (intptr_t)INTERRUPT_TASK_PTR)
        .withParameter(
            "msg", "event_delay_task(): no task or interrupt is running.");

    (void)interrupt_lock();

    CHECK_THROWS(AssertionError, event_delay_task(&event, MAX_DELAY));
}

TEST(Event, TaskResumedOnEvent_ShouldBeScheduled) {
    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);

    set_current_task(&task1);
    event_delay_task(&event, MAX_DELAY);
    event_resume_task(&event);
    set_current_task(NO_TASK_PTR);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("ABC", buff);
}

TEST(Event, NoTaskToResume_OK) {
    event_resume_task(&event);
}

TEST(Event, TwoTasksSuspendOnTheSameEvent_BothSuspend) {
    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);
    librertos_create_task(
        HIGH_PRIORITY, &task2, &Param::task_sequencing, &param2);

    librertos_start();

    set_current_task(&task1);
    event_delay_task(&event, MAX_DELAY);

    set_current_task(&task2);
    event_delay_task(&event, MAX_DELAY);

    set_current_task(NO_TASK_PTR);

    list_tester(
        &event.suspended_tasks,
        std::vector<node_t *>{&task2.event_node, &task1.event_node});

    librertos_sched();
    STRCMP_EQUAL("", buff);
}

TEST(Event, TwoTasksSuspendOnTheSameEvent_HigherPriorityResumesFirst) {
    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);
    librertos_create_task(
        HIGH_PRIORITY, &task2, &Param::task_sequencing, &param2);

    librertos_start();

    set_current_task(&task2);
    event_delay_task(&event, MAX_DELAY);

    set_current_task(&task1);
    event_delay_task(&event, MAX_DELAY);

    set_current_task(NO_TASK_PTR);

    list_tester(
        &event.suspended_tasks,
        std::vector<node_t *>{&task2.event_node, &task1.event_node});

    librertos_sched();
    STRCMP_EQUAL("", buff);

    event_resume_task(&event);
    librertos_sched();
    STRCMP_EQUAL("abc", buff);

    list_tester(
        &event.suspended_tasks, std::vector<node_t *>{&task1.event_node});

    event_resume_task(&event);
    librertos_sched();
    STRCMP_EQUAL("abcABC", buff);

    list_tester(&event.suspended_tasks, std::vector<node_t *>{});
}

TEST(Event, TwoTasksSuspendOnTheSameEvent_HigherPriorityResumesFirst_2) {
    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);
    librertos_create_task(
        HIGH_PRIORITY, &task2, &Param::task_sequencing, &param2);

    librertos_start();

    set_current_task(&task1);
    event_delay_task(&event, MAX_DELAY);

    set_current_task(&task2);
    event_delay_task(&event, MAX_DELAY);

    set_current_task(NO_TASK_PTR);

    list_tester(
        &event.suspended_tasks,
        std::vector<node_t *>{&task2.event_node, &task1.event_node});

    librertos_sched();
    STRCMP_EQUAL("", buff);

    event_resume_task(&event);
    librertos_sched();
    STRCMP_EQUAL("abc", buff);

    list_tester(
        &event.suspended_tasks, std::vector<node_t *>{&task1.event_node});

    event_resume_task(&event);
    librertos_sched();
    STRCMP_EQUAL("abcABC", buff);

    list_tester(&event.suspended_tasks, std::vector<node_t *>{});
}

TEST(Event, ThreeTasksSuspendOnTheSameEvent_HigherPriorityResumesFirst) {
    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);
    librertos_create_task(
        LOW_PRIORITY + 1, &task2, &Param::task_sequencing, &param2);
    librertos_create_task(
        HIGH_PRIORITY, &task3, &Param::task_sequencing, &param3);

    librertos_start();

    set_current_task(&task3);
    event_delay_task(&event, MAX_DELAY);

    set_current_task(&task2);
    event_delay_task(&event, MAX_DELAY);

    set_current_task(&task1);
    event_delay_task(&event, MAX_DELAY);

    set_current_task(NO_TASK_PTR);

    list_tester(
        &event.suspended_tasks,
        std::vector<node_t *>{
            &task3.event_node, &task2.event_node, &task1.event_node});

    librertos_sched();
    STRCMP_EQUAL("", buff);

    event_resume_task(&event);
    librertos_sched();
    STRCMP_EQUAL("123", buff);

    event_resume_task(&event);
    librertos_sched();
    STRCMP_EQUAL("123abc", buff);

    event_resume_task(&event);
    librertos_sched();
    STRCMP_EQUAL("123abcABC", buff);
}

TEST(Event, ThreeTasksSuspendOnTheSameEvent_HigherPriorityResumesFirst_2) {
    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);
    librertos_create_task(
        LOW_PRIORITY + 1, &task2, &Param::task_sequencing, &param2);
    librertos_create_task(
        HIGH_PRIORITY, &task3, &Param::task_sequencing, &param3);

    librertos_start();

    set_current_task(&task3);
    event_delay_task(&event, MAX_DELAY);

    set_current_task(&task1);
    event_delay_task(&event, MAX_DELAY);

    set_current_task(&task2);
    event_delay_task(&event, MAX_DELAY);

    set_current_task(NO_TASK_PTR);

    list_tester(
        &event.suspended_tasks,
        std::vector<node_t *>{
            &task3.event_node, &task2.event_node, &task1.event_node});

    librertos_sched();
    STRCMP_EQUAL("", buff);

    event_resume_task(&event);
    librertos_sched();
    STRCMP_EQUAL("123", buff);

    event_resume_task(&event);
    librertos_sched();
    STRCMP_EQUAL("123abc", buff);

    event_resume_task(&event);
    librertos_sched();
    STRCMP_EQUAL("123abcABC", buff);
}

TEST(Event, ThreeTasksSuspendOnTheSameEvent_HigherPriorityResumesFirst_3) {
    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);
    librertos_create_task(
        LOW_PRIORITY + 1, &task2, &Param::task_sequencing, &param2);
    librertos_create_task(
        HIGH_PRIORITY, &task3, &Param::task_sequencing, &param3);

    librertos_start();

    set_current_task(&task2);
    event_delay_task(&event, MAX_DELAY);

    set_current_task(&task3);
    event_delay_task(&event, MAX_DELAY);

    set_current_task(&task1);
    event_delay_task(&event, MAX_DELAY);

    set_current_task(NO_TASK_PTR);

    list_tester(
        &event.suspended_tasks,
        std::vector<node_t *>{
            &task3.event_node, &task2.event_node, &task1.event_node});

    librertos_sched();
    STRCMP_EQUAL("", buff);

    event_resume_task(&event);
    librertos_sched();
    STRCMP_EQUAL("123", buff);

    event_resume_task(&event);
    librertos_sched();
    STRCMP_EQUAL("123abc", buff);

    event_resume_task(&event);
    librertos_sched();
    STRCMP_EQUAL("123abcABC", buff);
}

TEST(Event, ThreeTasksSuspendOnTheSameEvent_HigherPriorityResumesFirst_4) {
    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);
    librertos_create_task(
        LOW_PRIORITY + 1, &task2, &Param::task_sequencing, &param2);
    librertos_create_task(
        HIGH_PRIORITY, &task3, &Param::task_sequencing, &param3);

    librertos_start();

    set_current_task(&task2);
    event_delay_task(&event, MAX_DELAY);

    set_current_task(&task1);
    event_delay_task(&event, MAX_DELAY);

    set_current_task(&task3);
    event_delay_task(&event, MAX_DELAY);

    set_current_task(NO_TASK_PTR);

    list_tester(
        &event.suspended_tasks,
        std::vector<node_t *>{
            &task3.event_node, &task2.event_node, &task1.event_node});

    librertos_sched();
    STRCMP_EQUAL("", buff);

    event_resume_task(&event);
    librertos_sched();
    STRCMP_EQUAL("123", buff);

    event_resume_task(&event);
    librertos_sched();
    STRCMP_EQUAL("123abc", buff);

    event_resume_task(&event);
    librertos_sched();
    STRCMP_EQUAL("123abcABC", buff);
}

TEST(Event, ThreeTasksSuspendOnTheSameEvent_HigherPriorityResumesFirst_5) {
    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);
    librertos_create_task(
        LOW_PRIORITY + 1, &task2, &Param::task_sequencing, &param2);
    librertos_create_task(
        HIGH_PRIORITY, &task3, &Param::task_sequencing, &param3);

    librertos_start();

    set_current_task(&task1);
    event_delay_task(&event, MAX_DELAY);

    set_current_task(&task3);
    event_delay_task(&event, MAX_DELAY);

    set_current_task(&task2);
    event_delay_task(&event, MAX_DELAY);

    set_current_task(NO_TASK_PTR);

    list_tester(
        &event.suspended_tasks,
        std::vector<node_t *>{
            &task3.event_node, &task2.event_node, &task1.event_node});

    librertos_sched();
    STRCMP_EQUAL("", buff);

    event_resume_task(&event);
    librertos_sched();
    STRCMP_EQUAL("123", buff);

    event_resume_task(&event);
    librertos_sched();
    STRCMP_EQUAL("123abc", buff);

    event_resume_task(&event);
    librertos_sched();
    STRCMP_EQUAL("123abcABC", buff);
}

TEST(Event, ThreeTasksSuspendOnTheSameEvent_HigherPriorityResumesFirst_6) {
    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);
    librertos_create_task(
        LOW_PRIORITY + 1, &task2, &Param::task_sequencing, &param2);
    librertos_create_task(
        HIGH_PRIORITY, &task3, &Param::task_sequencing, &param3);

    librertos_start();

    set_current_task(&task1);
    event_delay_task(&event, MAX_DELAY);

    set_current_task(&task2);
    event_delay_task(&event, MAX_DELAY);

    set_current_task(&task3);
    event_delay_task(&event, MAX_DELAY);

    set_current_task(NO_TASK_PTR);

    list_tester(
        &event.suspended_tasks,
        std::vector<node_t *>{
            &task3.event_node, &task2.event_node, &task1.event_node});

    librertos_sched();
    STRCMP_EQUAL("", buff);

    event_resume_task(&event);
    librertos_sched();
    STRCMP_EQUAL("123", buff);

    event_resume_task(&event);
    librertos_sched();
    STRCMP_EQUAL("123abc", buff);

    event_resume_task(&event);
    librertos_sched();
    STRCMP_EQUAL("123abcABC", buff);
}

TEST_GROUP (EventNewTest) {
    event_t event;

    void setup() {
        test_init();
        event_init(&event);
    }
    void teardown() {
    }
};

TEST(EventNewTest, TaskSuspendsOnEvent_ResumesWithTaskResume) {
    test_create_tasks({0}, NULL, {NULL});

    set_current_task(&test.task[0]);
    event_delay_task(&event, MAX_DELAY);

    test_task_is_suspended(&test.task[0]);

    task_resume(&test.task[0]);

    test_task_is_ready(&test.task[0]);
}

TEST(EventNewTest, TaskSuspendsOnEvent_ResumesWithEvent) {
    test_create_tasks({0}, NULL, {NULL});

    set_current_task(&test.task[0]);
    event_delay_task(&event, MAX_DELAY);

    test_task_is_suspended(&test.task[0]);

    event_resume_task(&event);

    test_task_is_ready(&test.task[0]);
}

TEST(EventNewTest, TaskDelaysOnEvent_ResumesWithTaskResume) {
    test_create_tasks({0}, NULL, {NULL});

    set_current_task(&test.task[0]);
    event_delay_task(&event, 1);

    test_task_is_delayed_current(&test.task[0]);

    task_resume(&test.task[0]);

    test_task_is_ready(&test.task[0]);
}

TEST(EventNewTest, TaskDelaysOnEvent_ResumesWithEvent) {
    test_create_tasks({0}, NULL, {NULL});

    set_current_task(&test.task[0]);
    event_delay_task(&event, 1);

    test_task_is_delayed_current(&test.task[0]);

    event_resume_task(&event);

    test_task_is_ready(&test.task[0]);
}

TEST(EventNewTest, TaskDelaysOnEvent_ResumesWithTickInterrupt) {
    test_create_tasks({0}, NULL, {NULL});

    set_current_task(&test.task[0]);
    event_delay_task(&event, 1);

    test_task_is_delayed_current(&test.task[0]);

    librertos_tick_interrupt();

    test_task_is_ready(&test.task[0]);
}

TEST(EventNewTest, TaskDelaysOnEvent_ResumesWithTickInterrupt_2) {
    test_create_tasks({0}, NULL, {NULL});

    set_current_task(&test.task[0]);
    event_delay_task(&event, 2);

    test_task_is_delayed_current(&test.task[0]);

    librertos_tick_interrupt();

    test_task_is_delayed_current(&test.task[0]);

    librertos_tick_interrupt();

    test_task_is_ready(&test.task[0]);
}

TEST(EventNewTest, TaskDelaysOnEvent_ResumesWithTickInterrupt_3) {
    test_create_tasks({0}, NULL, {NULL});

    set_tick(3);
    set_current_task(&test.task[0]);
    event_delay_task(&event, MAX_DELAY - 1);

    test_task_is_delayed_overflow(&test.task[0]);

    set_tick(MAX_DELAY - 1);
    librertos_tick_interrupt();

    test_task_is_delayed_overflow(&test.task[0]);

    librertos_tick_interrupt();

    test_task_is_delayed_current(&test.task[0]);

    librertos_tick_interrupt();

    test_task_is_ready(&test.task[0]);
}
