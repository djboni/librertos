/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos.h"
#include "librertos_impl.h"
#include "tests/utils/librertos_test_utils.h"

/*
 * Main file: src/mutex.c
 * Also compile: src/librertos.c
 * Also compile: tests/mocks/librertos_assert.cpp
 * Also compile: tests/utils/librertos_test_utils.cpp
 */

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

TEST_GROUP (Mutex)
{
    mutex_t mtx;
    task_t task1, task2;

    void setup() { mutex_init(&mtx); }
    void teardown() {}
};

TEST(Mutex, Unlocked_Locks)
{
    LONGS_EQUAL(SUCCESS, mutex_lock(&mtx));
}

TEST(Mutex, TryUnlockAnUnlockedMutex_CallsAssertFunction)
{
    mock()
        .expectOneCall("librertos_assert")
        .withParameter("val", 0)
        .withParameter("msg", "mutex_unlock(): mutex already unlocked.");

    CHECK_THROWS(AssertionError, mutex_unlock(&mtx));
}

TEST(Mutex, Locked_Unlocks)
{
    mutex_lock(&mtx);
    mutex_unlock(&mtx);
}

TEST(Mutex, IsLocked_SameTask)
{
    LONGS_EQUAL(0, mutex_is_locked(&mtx));

    mutex_lock(&mtx);
    LONGS_EQUAL(0, mutex_is_locked(&mtx));

    mutex_lock(&mtx);
    LONGS_EQUAL(0, mutex_is_locked(&mtx));
}

TEST(Mutex, IsLocked_OtherTask)
{
    LONGS_EQUAL(0, mutex_is_locked(&mtx));

    set_current_task(&task1);
    mutex_lock(&mtx);
    set_current_task(NO_TASK_PTR);

    LONGS_EQUAL(1, mutex_is_locked(&mtx));

    set_current_task(&task1);
    mutex_lock(&mtx);
    set_current_task(NO_TASK_PTR);

    LONGS_EQUAL(1, mutex_is_locked(&mtx));
}

TEST(Mutex, Unlocked_TaskCanLockMultipleTimes)
{
    librertos_init();

    scheduler_lock();
    set_current_task(&task1);

    LONGS_EQUAL(SUCCESS, mutex_lock(&mtx));
    LONGS_EQUAL(SUCCESS, mutex_lock(&mtx));
}

TEST(Mutex, Locked_TaskCanUnLockMultipleTimes)
{
    librertos_init();

    scheduler_lock();
    set_current_task(&task1);
    mutex_lock(&mtx);
    mutex_lock(&mtx);

    mutex_unlock(&mtx);
    mutex_unlock(&mtx);
}

TEST(Mutex, Unlocked_TaskCanLock_AndUnlock_MultipleTimes)
{
    librertos_init();

    scheduler_lock();
    set_current_task(&task1);

    LONGS_EQUAL(SUCCESS, mutex_lock(&mtx));
    mutex_unlock(&mtx);

    LONGS_EQUAL(SUCCESS, mutex_lock(&mtx));
    LONGS_EQUAL(SUCCESS, mutex_lock(&mtx));
    mutex_unlock(&mtx);
    mutex_unlock(&mtx);

    LONGS_EQUAL(SUCCESS, mutex_lock(&mtx));
    LONGS_EQUAL(SUCCESS, mutex_lock(&mtx));
    LONGS_EQUAL(SUCCESS, mutex_lock(&mtx));
    mutex_unlock(&mtx);
    mutex_unlock(&mtx);
    mutex_unlock(&mtx);
}

TEST(Mutex, Locked_SecondTaskCannotLock)
{
    librertos_init();

    scheduler_lock();
    set_current_task(&task1);
    mutex_lock(&mtx);
    set_current_task(&task2);

    LONGS_EQUAL(FAIL, mutex_lock(&mtx));
}

TEST(Mutex, Locked_InterruptCannotLock)
{
    librertos_init();

    scheduler_lock();
    set_current_task(&task1);
    mutex_lock(&mtx);
    (void)interrupt_lock();

    LONGS_EQUAL(FAIL, mutex_lock(&mtx));
}

TEST(Mutex, Unlocked_InterruptCanLockMultipleTimes)
{
    librertos_init();

    (void)interrupt_lock();

    LONGS_EQUAL(SUCCESS, mutex_lock(&mtx));
    mutex_unlock(&mtx);

    LONGS_EQUAL(SUCCESS, mutex_lock(&mtx));
    LONGS_EQUAL(SUCCESS, mutex_lock(&mtx));
    mutex_unlock(&mtx);
    mutex_unlock(&mtx);

    LONGS_EQUAL(SUCCESS, mutex_lock(&mtx));
    LONGS_EQUAL(SUCCESS, mutex_lock(&mtx));
    LONGS_EQUAL(SUCCESS, mutex_lock(&mtx));
    mutex_unlock(&mtx);
    mutex_unlock(&mtx);
    mutex_unlock(&mtx);
}

TEST_GROUP (MutexEvent)
{
    char buff[BUFF_SIZE];

    mutex_t mtx;
    task_t task1, task2;
    ParamMutex param1;

    void setup()
    {
        // Task sequencing buffer
        strcpy(buff, "");

        // Task parameters
        param1 = ParamMutex{&buff[0], &mtx, "A", "L", "U", "_"};

        librertos_init();
        mutex_init(&mtx);
    }
    void teardown() {}
};

TEST(MutexEvent, TaskSuspendsOnEvent_ShouldNotBeScheduled)
{
    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamMutex::task_sequencing, &param1);

    mutex_lock(&mtx);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("AL_", buff);

    librertos_sched();
    STRCMP_EQUAL("AL_", buff);
}

TEST(MutexEvent, TaskSuspendsOnAvailableEvent_ShouldBeScheduled)
{
    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamMutex::task_sequencing, &param1);

    set_current_task(&task1);
    mutex_suspend(&mtx);
    set_current_task(NO_TASK_PTR);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("AU_", buff);
}

TEST(MutexEvent, TaskResumesOnEvent_ShouldBeScheduled)
{
    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamMutex::task_sequencing, &param1);

    mutex_lock(&mtx);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("AL_", buff);

    mutex_unlock(&mtx);
    STRCMP_EQUAL("AL_AU_", buff);
}

TEST(MutexEvent, TaskResumesOnEvent_SchedulerLocked_ShouldNotBeScheduled)
{
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

TEST(MutexEvent, TaskLockSuspend_UnavailableMutex_ShouldNotBeScheduled)
{
    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamMutex::task_mutex_lock_suspend, &param1);
    librertos_create_task(
        LOW_PRIORITY, &task2, &ParamMutex::task_mutex_lock_suspend, NULL);

    librertos_start();

    set_current_task(&task2);
    mutex_lock(&mtx);
    set_current_task(NO_TASK_PTR);

    librertos_sched();

    STRCMP_EQUAL(
        "AL_", // Suspends
        buff);
}

TEST(MutexEvent, TaskLockSuspend_UnavailableMutex_ShouldBeScheduledWhenUnlocked)
{
    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamMutex::task_mutex_lock_suspend, &param1);

    librertos_start();

    set_current_task(&task2);
    mutex_lock(&mtx);
    set_current_task(NO_TASK_PTR);

    librertos_sched();

    STRCMP_EQUAL(
        "AL_", // Suspends
        buff);

    mutex_unlock(&mtx);

    STRCMP_EQUAL(
        "AL_"  // Suspends (previous)
        "AU_", // Locks and suspends
        buff);
}

TEST(MutexEvent, TaskLockSuspend_AvailableMutex_ShouldBeScheduled)
{
    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamMutex::task_mutex_lock_suspend, &param1);

    librertos_start();
    librertos_sched();

    STRCMP_EQUAL(
        "AU_", // Locks and suspends
        buff);
}
