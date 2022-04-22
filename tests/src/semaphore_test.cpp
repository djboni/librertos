/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos.h"
#include "tests/utils/librertos_custom_tests.h"
#include "tests/utils/librertos_test_utils.h"

/*
 * Main file: src/semaphore.c
 * Also compile: src/librertos.c
 * Also compile: tests/mocks/librertos_assert.cpp
 * Also compile: tests/utils/librertos_test_utils.cpp
 * Also compile: tests/utils/librertos_custom_tests.cpp
 */

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

TEST_GROUP (Sempahore)
{
    semaphore_t sem;

    void setup() {}
    void teardown() {}
};

TEST(Sempahore, BinaryLocked_CannotLock)
{
    semaphore_init_locked(&sem, 1);
    LONGS_EQUAL(FAIL, semaphore_lock(&sem));
}

TEST(Sempahore, BinaryUnlocked_Locks)
{
    semaphore_init_unlocked(&sem, 1);
    LONGS_EQUAL(SUCCESS, semaphore_lock(&sem));
}

TEST(Sempahore, BinaryUnlocked_CannotLockTwice)
{
    semaphore_init_unlocked(&sem, 1);
    semaphore_lock(&sem);
    LONGS_EQUAL(FAIL, semaphore_lock(&sem));
}

TEST(Sempahore, BinaryLocked_CanUnlock)
{
    semaphore_init_locked(&sem, 1);
    LONGS_EQUAL(SUCCESS, semaphore_unlock(&sem));
}

TEST(Sempahore, BinaryLocked_CannotUnlock)
{
    semaphore_init_unlocked(&sem, 1);
    LONGS_EQUAL(FAIL, semaphore_unlock(&sem));
}

TEST(Sempahore, BinaryLocked_CannotUnlockTwice)
{
    semaphore_init_locked(&sem, 1);
    semaphore_unlock(&sem);
    LONGS_EQUAL(FAIL, semaphore_unlock(&sem));
}

TEST(Sempahore, BinaryLocked_CannotLock_CanUnlock)
{
    semaphore_init_locked(&sem, 1);
    LONGS_EQUAL(FAIL, semaphore_lock(&sem));
    LONGS_EQUAL(SUCCESS, semaphore_unlock(&sem));
}

TEST(Sempahore, CountingLocked_CannotLock)
{
    semaphore_init_locked(&sem, 2);
    LONGS_EQUAL(FAIL, semaphore_lock(&sem));
}

TEST(Sempahore, CountingUnlocked_Locks)
{
    semaphore_init_unlocked(&sem, 2);
    LONGS_EQUAL(SUCCESS, semaphore_lock(&sem));
}

TEST(Sempahore, CountingUnlocked_LocksTwice)
{
    semaphore_init_unlocked(&sem, 2);
    semaphore_lock(&sem);
    LONGS_EQUAL(SUCCESS, semaphore_lock(&sem));
}

TEST(Sempahore, CountingUnlocked_CannotLockThreeTimes)
{
    semaphore_init_unlocked(&sem, 2);
    semaphore_lock(&sem);
    semaphore_lock(&sem);
    LONGS_EQUAL(FAIL, semaphore_lock(&sem));
}

TEST(Sempahore, CountingLocked_CanUnlock)
{
    semaphore_init_locked(&sem, 2);
    LONGS_EQUAL(SUCCESS, semaphore_unlock(&sem));
}

TEST(Sempahore, CountingLocked_CannotUnlock)
{
    semaphore_init_unlocked(&sem, 2);
    LONGS_EQUAL(FAIL, semaphore_unlock(&sem));
}

TEST(Sempahore, CountingLocked_CannotUnlockTwice)
{
    semaphore_init_locked(&sem, 2);
    semaphore_unlock(&sem);
    LONGS_EQUAL(SUCCESS, semaphore_unlock(&sem));
}

TEST(Sempahore, CountingLocked_CannotUnlockThreeTimes)
{
    semaphore_init_locked(&sem, 2);
    semaphore_unlock(&sem);
    semaphore_unlock(&sem);
    LONGS_EQUAL(FAIL, semaphore_unlock(&sem));
}

TEST(Sempahore, Binary_GetCount_GetMax)
{
    semaphore_init_locked(&sem, 1);
    LONGS_EQUAL(0, semaphore_get_count(&sem));
    LONGS_EQUAL(1, semaphore_get_max(&sem));

    semaphore_unlock(&sem);
    LONGS_EQUAL(1, semaphore_get_count(&sem));
    LONGS_EQUAL(1, semaphore_get_max(&sem));
}

TEST(Sempahore, Counting_GetCount_GetMax)
{
    semaphore_init_locked(&sem, 2);
    LONGS_EQUAL(0, semaphore_get_count(&sem));
    LONGS_EQUAL(2, semaphore_get_max(&sem));

    semaphore_unlock(&sem);
    LONGS_EQUAL(1, semaphore_get_count(&sem));
    LONGS_EQUAL(2, semaphore_get_max(&sem));

    semaphore_unlock(&sem);
    LONGS_EQUAL(2, semaphore_get_count(&sem));
    LONGS_EQUAL(2, semaphore_get_max(&sem));
}

TEST(Sempahore, InvalidInitCount_CallsAssertFunction)
{
    mock()
        .expectOneCall("librertos_assert")
        .withParameter("val", 2)
        .withParameter("msg", "semaphore_init(): invalid init_count.");

    CHECK_THROWS(AssertionError, semaphore_init(&sem, 2, 1));
}

TEST_GROUP (SemaphoreEvent)
{
    char buff[BUFF_SIZE];

    semaphore_t sem;
    task_t task1, task2;
    ParamSemaphore param1;

    void setup()
    {
        // Task sequencing buffer
        strcpy(buff, "");

        // Task parameters
        param1 = ParamSemaphore{&buff[0], &sem, "A", "L", "U", "_"};

        // Initialize
        librertos_init();
        semaphore_init_locked(&sem, 1);
    }
    void teardown() {}
};

TEST(SemaphoreEvent, TaskSuspendsOnEvent_ShouldNotBeScheduled)
{
    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamSemaphore::task_sequencing, &param1);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("AL_", buff);

    librertos_sched();
    STRCMP_EQUAL("AL_", buff);
}

TEST(SemaphoreEvent, TaskSuspendsOnAvailableEvent_ShouldBeScheduled)
{
    semaphore_unlock(&sem);

    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamSemaphore::task_sequencing, &param1);

    set_current_task(&task1);
    semaphore_suspend(&sem, MAX_DELAY);
    set_current_task(NO_TASK_PTR);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("AU_", buff);
}

TEST(SemaphoreEvent, TaskResumesOnEvent_ShouldBeScheduled)
{
    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamSemaphore::task_sequencing, &param1);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("AL_", buff);

    semaphore_unlock(&sem);
    STRCMP_EQUAL("AL_AU_", buff);
}

TEST(SemaphoreEvent, TaskResumesOnEvent_SchedulerLocked_ShouldNotBeScheduled)
{
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

TEST(SemaphoreEvent, TaskLockSuspend_UnavailableSemaphore_ShouldNotBeScheduled)
{
    librertos_create_task(
        LOW_PRIORITY,
        &task1,
        &ParamSemaphore::task_semaphore_lock_suspend,
        &param1);

    librertos_start();
    librertos_sched();

    STRCMP_EQUAL(
        "AL_", // Suspends
        buff);
}

TEST(
    SemaphoreEvent,
    TaskLockSuspend_UnavailableSemaphore_ShouldBeScheduledWhenUnlocked)
{
    librertos_create_task(
        LOW_PRIORITY,
        &task1,
        &ParamSemaphore::task_semaphore_lock_suspend,
        &param1);

    librertos_start();
    librertos_sched();

    STRCMP_EQUAL(
        "AL_", // Suspends
        buff);

    semaphore_unlock(&sem);

    STRCMP_EQUAL(
        "AL_"  // Suspends (previous)
        "AU_"  // Locks
        "AL_", // Suspends
        buff);
}

TEST(SemaphoreEvent, TaskLockSuspend_AvailableSemaphore_ShouldBeScheduled)
{
    librertos_create_task(
        LOW_PRIORITY,
        &task1,
        &ParamSemaphore::task_semaphore_lock_suspend,
        &param1);

    semaphore_unlock(&sem);
    librertos_start();
    librertos_sched();

    STRCMP_EQUAL(
        "AU_"  // Locks
        "AL_", // Suspends
        buff);
}

TEST_GROUP (SemaphoreEventNewTest)
{
    semaphore_t sem;

    void setup()
    {
        test_init();
        semaphore_init_locked(&sem, 1);
    }
    void teardown() {}
};

TEST(SemaphoreEventNewTest, TaskSuspendsOnEvent_ResumesWithTaskResume)
{
    test_create_tasks({0}, NULL, {NULL});

    set_current_task(&test.task[0]);
    semaphore_suspend(&sem, MAX_DELAY);

    test_task_is_suspended(&test.task[0]);

    task_resume(&test.task[0]);

    test_task_is_ready(&test.task[0]);
}

TEST(SemaphoreEventNewTest, TaskSuspendsOnEvent_ResumesWithEvent)
{
    test_create_tasks({0}, NULL, {NULL});

    set_current_task(&test.task[0]);
    semaphore_suspend(&sem, MAX_DELAY);

    test_task_is_suspended(&test.task[0]);

    semaphore_unlock(&sem);

    test_task_is_ready(&test.task[0]);
}

TEST(SemaphoreEventNewTest, TaskDelaysOnEvent_ResumesWithTaskResume)
{
    test_create_tasks({0}, NULL, {NULL});

    set_current_task(&test.task[0]);
    semaphore_suspend(&sem, 1);

    test_task_is_delayed_current(&test.task[0]);

    task_resume(&test.task[0]);

    test_task_is_ready(&test.task[0]);
}

TEST(SemaphoreEventNewTest, TaskDelaysOnEvent_ResumesWithEvent)
{
    test_create_tasks({0}, NULL, {NULL});

    set_current_task(&test.task[0]);
    semaphore_suspend(&sem, 1);

    test_task_is_delayed_current(&test.task[0]);

    semaphore_unlock(&sem);

    test_task_is_ready(&test.task[0]);
}

TEST(SemaphoreEventNewTest, TaskDelaysOnEvent_ResumesWithTickInterrupt)
{
    test_create_tasks({0}, NULL, {NULL});

    set_current_task(&test.task[0]);
    semaphore_suspend(&sem, 1);

    test_task_is_delayed_current(&test.task[0]);

    librertos_tick_interrupt();

    test_task_is_ready(&test.task[0]);
}

TEST(SemaphoreEventNewTest, TaskLockSuspendOnEvent_Success)
{
    test_create_tasks({0}, NULL, {NULL});
    semaphore_unlock(&sem);

    set_current_task(&test.task[0]);

    LONGS_EQUAL(1, semaphore_lock_suspend(&sem, MAX_DELAY));
    test_task_is_ready(&test.task[0]);
}

TEST(SemaphoreEventNewTest, TaskLockSuspendOnEvent_Fail)
{
    test_create_tasks({0}, NULL, {NULL});

    set_current_task(&test.task[0]);

    LONGS_EQUAL(0, semaphore_lock_suspend(&sem, MAX_DELAY));
    test_task_is_suspended(&test.task[0]);
}

TEST(SemaphoreEventNewTest, TaskLockDelaysOnEvent_Fail)
{
    test_create_tasks({0}, NULL, {NULL});

    set_current_task(&test.task[0]);

    LONGS_EQUAL(0, semaphore_lock_suspend(&sem, 1));
    test_task_is_delayed_current(&test.task[0]);
}
