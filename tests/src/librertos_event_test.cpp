/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos.h"
#include "librertos_impl.h"
#include "tests/utils/librertos_test_utils.h"

/*
 * Main file: src/librertos.c
 * Also compile: src/semaphore.c
 * Also compile: src/mutex.c
 * Also compile: src/queue.c
 * Also compile: tests/mocks/librertos_assert.cpp
 * Also compile: tests/utils/librertos_test_utils.cpp
 */

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

TEST_GROUP (Event)
{
    event_t event;
    task_t task1, task2;

    void setup()
    {
        librertos_init();
        event_init(&event);
    }
    void teardown() {}
};

TEST(Event, TaskSuspendsOnEvent_ShouldNotBeScheduled)
{
    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", NULL, 1};

    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);

    set_current_task(&task1);
    event_suspend_task(&event);

    librertos_sched();
    STRCMP_EQUAL("", buff);
}

TEST(Event, TaskSuspendsTwiceOnEvent_ShouldNotBeScheduled)
{
    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", NULL, 1};

    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);

    set_current_task(&task1);
    event_suspend_task(&event);
    event_suspend_task(&event);

    librertos_sched();
    STRCMP_EQUAL("", buff);
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
    char buff[BUFF_SIZE] = "";
    Param param1 = {&buff[0], "A", "B", "C", NULL, 1};

    librertos_create_task(
        LOW_PRIORITY, &task1, &Param::task_sequencing, &param1);

    set_current_task(&task1);
    event_suspend_task(&event);
    event_resume_task(&event);
    set_current_task(NO_TASK_PTR);

    librertos_sched();
    STRCMP_EQUAL("ABC", buff);
}

TEST(Event, NoTaskToResume_OK)
{
    event_resume_task(&event);
}

TEST_GROUP (Semaphore)
{
    semaphore_t sem;
    task_t task1, task2;

    void setup()
    {
        librertos_init();
        semaphore_init_locked(&sem, 1);
    }
    void teardown() {}
};

TEST(Semaphore, TaskSuspendsOnEvent_ShouldNotBeScheduled)
{
    char buff[BUFF_SIZE] = "";
    ParamSemaphore param1 = {&buff[0], &sem, "A", "L", "U", "_"};

    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamSemaphore::task_sequencing, &param1);

    librertos_sched();
    STRCMP_EQUAL("AL_", buff);

    librertos_sched();
    STRCMP_EQUAL("AL_", buff);
}

TEST(Semaphore, TaskSuspendsOnAvailableEvent_ShouldBeScheduled)
{
    char buff[BUFF_SIZE] = "";
    ParamSemaphore param1 = {&buff[0], &sem, "A", "L", "U", "_"};

    semaphore_unlock(&sem);

    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamSemaphore::task_sequencing, &param1);

    set_current_task(&task1);
    semaphore_suspend(&sem);
    set_current_task(NO_TASK_PTR);

    librertos_sched();
    STRCMP_EQUAL("AU_", buff);
}

TEST(Semaphore, TaskResumesOnEvent_ShouldBeScheduled)
{
    char buff[BUFF_SIZE] = "";
    ParamSemaphore param1 = {&buff[0], &sem, "A", "L", "U", "_"};

    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamSemaphore::task_sequencing, &param1);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("AL_", buff);

    semaphore_unlock(&sem);
    STRCMP_EQUAL("AL_AU_", buff);
}

TEST(Semaphore, TaskResumesOnEvent_SchedulerLocked_ShouldNotBeScheduled)
{
    char buff[BUFF_SIZE] = "";
    ParamSemaphore param1 = {&buff[0], &sem, "A", "L", "U", "_"};

    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamSemaphore::task_sequencing, &param1);

    librertos_start();
    librertos_sched();

    scheduler_lock();
    semaphore_unlock(&sem);
    STRCMP_EQUAL("AL_", buff);
    scheduler_unlock();

    STRCMP_EQUAL("AL_AU_", buff);
}

TEST_GROUP (Mutex)
{
    mutex_t mtx;
    task_t task1, task2;

    void setup()
    {
        librertos_init();
        mutex_init(&mtx);
    }
    void teardown() {}
};

TEST(Mutex, TaskSuspendsOnEvent_ShouldNotBeScheduled)
{
    char buff[BUFF_SIZE] = "";
    ParamMutex param1 = {&buff[0], &mtx, "A", "L", "U", "_"};

    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamMutex::task_sequencing, &param1);

    mutex_lock(&mtx);

    librertos_sched();
    STRCMP_EQUAL("AL_", buff);

    librertos_sched();
    STRCMP_EQUAL("AL_", buff);
}

TEST(Mutex, TaskSuspendsOnAvailableEvent_ShouldBeScheduled)
{
    char buff[BUFF_SIZE] = "";
    ParamMutex param1 = {&buff[0], &mtx, "A", "L", "U", "_"};

    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamMutex::task_sequencing, &param1);

    set_current_task(&task1);
    mutex_suspend(&mtx);
    set_current_task(NO_TASK_PTR);

    librertos_sched();
    STRCMP_EQUAL("AU_", buff);
}

TEST(Mutex, TaskResumesOnEvent_ShouldBeScheduled)
{
    char buff[BUFF_SIZE] = "";
    ParamMutex param1 = {&buff[0], &mtx, "A", "L", "U", "_"};

    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamMutex::task_sequencing, &param1);

    mutex_lock(&mtx);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("AL_", buff);

    mutex_unlock(&mtx);
    STRCMP_EQUAL("AL_AU_", buff);
}

TEST(Mutex, TaskResumesOnEvent_SchedulerLocked_ShouldNotBeScheduled)
{
    char buff[BUFF_SIZE] = "";
    ParamMutex param1 = {&buff[0], &mtx, "A", "L", "U", "_"};

    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamMutex::task_sequencing, &param1);

    mutex_lock(&mtx);

    librertos_start();
    librertos_sched();

    scheduler_lock();
    mutex_unlock(&mtx);
    STRCMP_EQUAL("AL_", buff);
    scheduler_unlock();

    STRCMP_EQUAL("AL_AU_", buff);
}

TEST_GROUP (Queue)
{
    queue_t que;
    int8_t que_buff[1];
    task_t task1, task2;

    void setup()
    {
        librertos_init();
        queue_init(
            &que,
            &que_buff[0],
            sizeof(que_buff) / sizeof(que_buff[0]),
            sizeof(que_buff[0]));
    }
    void teardown() {}
};

TEST(Queue, TaskSuspendsOnEvent_ShouldNotBeScheduled)
{
    char buff[BUFF_SIZE] = "";
    ParamQueue param1 = {&buff[0], &que, "A", "L", "U", "_"};

    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamQueue::task_sequencing, &param1);

    librertos_sched();
    STRCMP_EQUAL("AL_", buff);

    librertos_sched();
    STRCMP_EQUAL("AL_", buff);
}

TEST(Queue, TaskSuspendsOnAvailableEvent_ShouldBeScheduled)
{
    char buff[BUFF_SIZE] = "";
    ParamQueue param1 = {&buff[0], &que, "A", "L", "U", "_"};
    int8_t data = 0;

    queue_write(&que, &data);

    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamQueue::task_sequencing, &param1);

    set_current_task(&task1);
    queue_suspend_read(&que);
    set_current_task(NO_TASK_PTR);

    librertos_sched();
    STRCMP_EQUAL("AU_", buff);
}

TEST(Queue, TaskResumesOnEvent_ShouldBeScheduled)
{
    char buff[BUFF_SIZE] = "";
    ParamQueue param1 = {&buff[0], &que, "A", "L", "U", "_"};
    int8_t data = 0;

    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamQueue::task_sequencing, &param1);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("AL_", buff);

    queue_write(&que, &data);
    STRCMP_EQUAL("AL_AU_", buff);
}

TEST(Queue, TaskResumesOnEvent_SchedulerLocked_ShouldNotBeScheduled)
{
    char buff[BUFF_SIZE] = "";
    ParamQueue param1 = {&buff[0], &que, "A", "L", "U", "_"};
    int8_t data = 0;

    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamQueue::task_sequencing, &param1);

    librertos_start();
    librertos_sched();

    scheduler_lock();
    queue_write(&que, &data);
    STRCMP_EQUAL("AL_", buff);
    scheduler_unlock();

    STRCMP_EQUAL("AL_AU_", buff);
}
