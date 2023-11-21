/* Copyright (c) 2016-2023 Djones A. Boni - MIT License */

#define LIBRERTOS_DEBUG_DECLARATIONS
#include "librertos.h"
#include "tests/utils/librertos_test_utils.h"

/*
 * Main file: src/librertos.c
 * Also compile: tests/mocks/librertos_assert.cpp
 * Also compile: tests/utils/librertos_test_utils.cpp
 */

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

static void timer_increments_param(timer_task_t *timer, void *param) {
    uint32_t *executed = (uint32_t *)param;
    (void)timer;
    *executed += 1;
}

TEST_GROUP (TimerAutoReset) {
    timer_task_t timer;

    void setup() {
        librertos_init();
    }
    void teardown() {
    }
};

TEST(TimerAutoReset, PeriodNotPassed_TimerDoesNotRun) {
    uint32_t executed = 0;

    librertos_create_timer(LOW_PRIORITY, &timer, &timer_increments_param, &executed,
        TIMERTYPE_AUTO, 1);

    librertos_start();

    librertos_sched();

    LONGS_EQUAL(0, executed);
}

TEST(TimerAutoReset, PeriodNotPassed_TimerDoesNotRun_2) {
    uint32_t executed = 0;
    task_t *interrupted_task;

    librertos_create_timer(LOW_PRIORITY, &timer, &timer_increments_param, &executed,
        TIMERTYPE_AUTO, 2);

    librertos_start();

    interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
    librertos_sched();

    LONGS_EQUAL(0, executed);
}

TEST(TimerAutoReset, PeriodPassed_TimerRuns) {
    uint32_t executed = 0;
    task_t *interrupted_task;

    librertos_create_timer(LOW_PRIORITY, &timer, &timer_increments_param, &executed,
        TIMERTYPE_AUTO, 1);

    librertos_start();

    interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
    librertos_sched();

    LONGS_EQUAL(1, executed);
}

TEST(TimerAutoReset, PeriodPassed_TimerRuns_2) {
    uint32_t executed = 0;
    task_t *interrupted_task;

    librertos_create_timer(LOW_PRIORITY, &timer, &timer_increments_param, &executed,
        TIMERTYPE_AUTO, 2);

    librertos_start();

    interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
    librertos_sched();

    interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
    librertos_sched();

    LONGS_EQUAL(1, executed);
}

TEST(TimerAutoReset, PeriodPassedTwice_TimerRunsTwice) {
    uint32_t executed = 0;
    task_t *interrupted_task;

    librertos_create_timer(LOW_PRIORITY, &timer, &timer_increments_param, &executed,
        TIMERTYPE_AUTO, 1);

    librertos_start();

    interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
    librertos_sched();

    interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
    librertos_sched();

    LONGS_EQUAL(2, executed);
}

TEST_GROUP (TimerOneShot) {
    timer_task_t timer;

    void setup() {
        librertos_init();
    }
    void teardown() {
    }
};

TEST(TimerOneShot, TimerNotStarted_TimerDoesNotRun) {
    uint32_t executed = 0;
    task_t *interrupted_task;

    librertos_create_timer(LOW_PRIORITY, &timer, &timer_increments_param, &executed,
        TIMERTYPE_ONESHOT, 1);

    librertos_start();

    interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
    librertos_sched();

    LONGS_EQUAL(0, executed);
}

TEST(TimerOneShot, TimerStarted_PeriodNotPassed_TimerDoesNotRun) {
    uint32_t executed = 0;

    librertos_create_timer(LOW_PRIORITY, &timer, &timer_increments_param, &executed,
        TIMERTYPE_ONESHOT, 1);

    librertos_start();

    timer_start(&timer);
    librertos_sched();

    LONGS_EQUAL(0, executed);
}

TEST(TimerOneShot, TimerStarted_PeriodPassed_TimerRuns) {
    uint32_t executed = 0;
    task_t *interrupted_task;

    librertos_create_timer(LOW_PRIORITY, &timer, &timer_increments_param, &executed,
        TIMERTYPE_ONESHOT, 1);

    librertos_start();

    timer_start(&timer);
    interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
    librertos_sched();

    LONGS_EQUAL(1, executed);
}

TEST(TimerOneShot, TimerStarted_PeriodPassed_TimerRuns_2) {
    uint32_t executed = 0;
    task_t *interrupted_task;

    librertos_create_timer(LOW_PRIORITY, &timer, &timer_increments_param, &executed,
        TIMERTYPE_ONESHOT, 2);

    librertos_start();

    timer_start(&timer);
    interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
    librertos_sched();

    interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
    librertos_sched();

    LONGS_EQUAL(1, executed);
}

TEST(TimerOneShot, TimerStartedOnce_PeriodPassedTwice_TimerRunsOnce) {
    uint32_t executed = 0;
    task_t *interrupted_task;

    librertos_create_timer(LOW_PRIORITY, &timer, &timer_increments_param, &executed,
        TIMERTYPE_ONESHOT, 1);

    librertos_start();

    timer_start(&timer);
    interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
    librertos_sched();

    interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
    librertos_sched();

    LONGS_EQUAL(1, executed);
}

TEST(TimerOneShot, TimerStartedTwice_PeriodPassedTwice_TimerRunsTwice) {
    uint32_t executed = 0;
    task_t *interrupted_task;

    librertos_create_timer(LOW_PRIORITY, &timer, &timer_increments_param, &executed,
        TIMERTYPE_ONESHOT, 1);

    librertos_start();

    timer_start(&timer);
    interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
    librertos_sched();

    timer_start(&timer);
    interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
    librertos_sched();

    LONGS_EQUAL(2, executed);
}

static void timer_resets_itself(timer_task_t *timer, void *param) {
    uint32_t *executed = (uint32_t *)param;
    *executed += 1;
    timer_reset(timer);
}

TEST(TimerOneShot, TimerStartsItself_TimerRunsIndefinitely) {
    uint32_t executed = 0;
    task_t *interrupted_task;

    librertos_create_timer(LOW_PRIORITY, &timer, &timer_resets_itself, &executed,
        TIMERTYPE_ONESHOT, 1);

    librertos_start();

    timer_start(&timer);
    interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
    librertos_sched();

    LONGS_EQUAL(1, executed);

    interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
    librertos_sched();

    LONGS_EQUAL(2, executed);
}

TEST_GROUP (Timer) {
    timer_task_t timer;

    void setup() {
        librertos_init();
    }
    void teardown() {
    }
};

TEST(Timer, TimerRunningWithInvalidType_CallsAssertFunction) {
    // The same is valid for one-shot timers.
    uint32_t executed = 0;
    task_t *interrupted_task;

    librertos_create_timer(LOW_PRIORITY, &timer, &timer_increments_param, &executed,
        TIMERTYPE_AUTO, 1);

    librertos_start();

    // Manually change librertos.scheduler_depth to force the timer to run
    // in CHECK_THROWS() call to librertos_sched().
    librertos.scheduler_depth++;
    interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
    librertos.scheduler_depth--;

    // Mess with the timer type to trigger the assertion
    timer.type = (timer_type_t)0;

    mock()
        .expectOneCall("librertos_assert")
        .withParameter(
            "msg", "Unreachable.");

    CHECK_THROWS(AssertionError, librertos_sched());
}

TEST(Timer, StartingARunningTimer_DoesNotChangeExpirePeriod) {
    uint32_t executed = 0;
    task_t *interrupted_task;

    librertos_create_timer(LOW_PRIORITY, &timer, &timer_increments_param, &executed,
        TIMERTYPE_ONESHOT, 2);

    librertos_start();

    timer_start(&timer);
    interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
    librertos_sched();

    LONGS_EQUAL(0, executed);

    timer_start(&timer);
    interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
    librertos_sched();

    LONGS_EQUAL(1, executed);
}

TEST(Timer, ResettingARunningTimer_ChangesTheExpirePeriod) {
    uint32_t executed = 0;
    task_t *interrupted_task;

    librertos_create_timer(LOW_PRIORITY, &timer, &timer_increments_param, &executed,
        TIMERTYPE_ONESHOT, 2);

    librertos_start();

    timer_start(&timer);
    interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
    librertos_sched();

    LONGS_EQUAL(0, executed);

    timer_reset(&timer);
    interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
    librertos_sched();

    LONGS_EQUAL(0, executed);

    interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
    librertos_sched();

    LONGS_EQUAL(1, executed);
}

TEST(Timer, StoppingARunningTimer_AvoidsItFromExecuting) {
    uint32_t executed = 0;
    task_t *interrupted_task;

    librertos_create_timer(LOW_PRIORITY, &timer, &timer_increments_param, &executed,
        TIMERTYPE_ONESHOT, 1);

    librertos_start();

    timer_start(&timer);
    timer_stop(&timer);
    interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
    librertos_sched();

    LONGS_EQUAL(0, executed);
}

TEST(Timer, StoppingAStoppedTimer_DoesNothing) {
    uint32_t executed = 0;
    task_t *interrupted_task;

    librertos_create_timer(LOW_PRIORITY, &timer, &timer_increments_param, &executed,
        TIMERTYPE_ONESHOT, 1);

    librertos_start();

    timer_stop(&timer);
    timer_stop(&timer);
    interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
    librertos_sched();

    LONGS_EQUAL(0, executed);
}

TEST(Timer, TaskResumeAll_DoesNotResumeRunningTimers) {
    uint32_t executed = 0;

    librertos_create_timer(LOW_PRIORITY, &timer, &timer_increments_param, &executed,
        TIMERTYPE_ONESHOT, 2);

    librertos_start();

    timer_start(&timer);
    task_resume_all();
    librertos_sched();

    LONGS_EQUAL(0, executed);
}

TEST(Timer, TaskResumeAll_DoesNotResumeStoppedTimers) {
    uint32_t executed = 0;

    librertos_create_timer(LOW_PRIORITY, &timer, &timer_increments_param, &executed,
        TIMERTYPE_ONESHOT, 2);

    librertos_start();

    task_resume_all();
    librertos_sched();

    LONGS_EQUAL(0, executed);
}
