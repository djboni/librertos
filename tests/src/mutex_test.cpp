/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos.h"
#include "librertos_impl.h"
#include "tests/utils/librertos_custom_tests.h"
#include "tests/utils/librertos_test_utils.h"

/*
 * Main file: src/mutex.c
 * Also compile: src/librertos.c
 * Also compile: tests/mocks/librertos_assert.cpp
 * Also compile: tests/utils/librertos_test_utils.cpp
 */

#include <vector>

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

TEST_GROUP (MutexPriorityInversion)
{
    mutex_t mtx;
    mutex_t helper;
    task_t task[4];
    uint8_t tasks_used;

    void setup() {}
    void teardown() {}
};

static void func(void *param)
{
    std::vector<task_t *> *sequence = (std::vector<task_t *> *)param;
    sequence->push_back(get_current_task());
    task_suspend(CURRENT_TASK_PTR);
}

TEST(
    MutexPriorityInversion,
    LowerPrioritySuspends_TaskReady_NoChangeInPriorities)
{
    std::vector<task_t *> actual_sequence;

    // Given A high priority task locks the mutex
    initialize_os(this);
    initialize_mutex(this);
    create_N_tasks_with_different_priorities(this, 3, &func, &actual_sequence);
    taskX_is_scheduled_and_locks_mutex(this, 2);

    // When A low priority task suspends on the mutex
    // And The scheduler is called
    taskX_is_scheduled_and_suspends_on_mutex(this, 0);
    call_the_scheduler(this);

    // Then The higher priority task should be scheduled
    // And The middle priority task should be scheduled
    tasks_should_be_scheduled_in_the_order(
        this, {&task[2], &task[1]}, actual_sequence);
}

TEST(
    MutexPriorityInversion,
    HigherPrioritySuspends_TaskReady_IncreasePriorityOfOwner)
{
    std::vector<task_t *> actual_sequence;

    // Given A low priority task locks the mutex
    initialize_os(this);
    initialize_mutex(this);
    create_N_tasks_with_different_priorities(this, 3, &func, &actual_sequence);
    taskX_is_scheduled_and_locks_mutex(this, 0);

    // When A high priority task suspends on the mutex
    // And The scheduler is called
    taskX_is_scheduled_and_suspends_on_mutex(this, 2);
    call_the_scheduler(this);

    // Then The lower priority task should be scheduled
    // And The middle priority task should be scheduled
    tasks_should_be_scheduled_in_the_order(
        this, {&task[0], &task[1]}, actual_sequence);
}

TEST(
    MutexPriorityInversion,
    HigherPrioritySuspends_TaskUnlocksMutex_OriginalPriorityReturns)
{
    std::vector<task_t *> actual_sequence;

    // Given A low priority task locks the mutex
    // And A high priority task suspends on the mutex
    initialize_os(this);
    initialize_mutex(this);
    create_N_tasks_with_different_priorities(this, 3, &func, &actual_sequence);
    taskX_is_scheduled_and_locks_mutex(this, 0);
    taskX_is_scheduled_and_suspends_on_mutex(this, 2);

    // When The mutex is unlocked
    // And The scheduler is called
    mutex_is_unlocked(this);
    call_the_scheduler(this);

    // Then The higher priority task should be scheduled
    // And The middle priority task should be scheduled
    // And The lower priority task should be scheduled
    tasks_should_be_scheduled_in_the_order(
        this, {&task[2], &task[1], &task[0]}, actual_sequence);
}

TEST(
    MutexPriorityInversion,
    LowerPrioritySuspends_TaskSuspendedOnEvent_NoChangeInPriorities)
{
    std::vector<task_t *> actual_sequence;

    // Given
    initialize_os(this);
    initialize_mutex(this);
    initialize_helper_mutex(this);
    create_N_tasks_with_different_priorities(this, 3, &func, &actual_sequence);
    taskX_is_scheduled_and_locks_mutex(this, 2);
    interrupt_locks_helper_mutex(this);
    taskX_is_scheduled_and_suspends_helper_on_mutex(this, 2);
    taskX_is_scheduled_and_suspends_helper_on_mutex(this, 1);

    // When
    taskX_is_scheduled_and_suspends_on_mutex(this, 0);
    helper_mutex_is_unlocked(this);
    call_the_scheduler(this);

    // Then
    tasks_should_be_scheduled_in_the_order(this, {&task[2]}, actual_sequence);

    // When
    interrupt_locks_helper_mutex(this);
    helper_mutex_is_unlocked(this);
    call_the_scheduler(this);

    // Then
    tasks_should_be_scheduled_in_the_order(
        this, {&task[2], &task[1]}, actual_sequence);
}

TEST(
    MutexPriorityInversion,
    HigherPrioritySuspends_TaskSuspendedOnEvent_IncreasePriorityOfOwner)
{
    std::vector<task_t *> actual_sequence;

    // Given
    initialize_os(this);
    initialize_mutex(this);
    initialize_helper_mutex(this);
    create_N_tasks_with_different_priorities(this, 3, &func, &actual_sequence);
    taskX_is_scheduled_and_locks_mutex(this, 0);
    interrupt_locks_helper_mutex(this);
    taskX_is_scheduled_and_suspends_helper_on_mutex(this, 0);
    taskX_is_scheduled_and_suspends_helper_on_mutex(this, 1);

    // When
    taskX_is_scheduled_and_suspends_on_mutex(this, 2);
    helper_mutex_is_unlocked(this);
    call_the_scheduler(this);

    // Then
    tasks_should_be_scheduled_in_the_order(this, {&task[0]}, actual_sequence);

    // When
    interrupt_locks_helper_mutex(this);
    helper_mutex_is_unlocked(this);
    call_the_scheduler(this);

    // Then
    tasks_should_be_scheduled_in_the_order(
        this, {&task[0], &task[1]}, actual_sequence);
}

static void func_lock_mutex_and_resume_1task(void *param)
{
    void **p = (void **)param;
    std::vector<task_t *> *sequence = (std::vector<task_t *> *)p[0];
    mutex_t *mutex = (mutex_t *)p[1];
    task_t *task1 = (task_t *)p[2];

    // Lock mutex
    mutex_lock(mutex);

    // Resume can preempt.
    task_resume(task1);

    // Sequence after possible preemption.
    sequence->push_back(get_current_task());
    task_suspend(CURRENT_TASK_PTR);
}

static void func_lock_mutex_and_resume_2tasks(void *param)
{
    void **p = (void **)param;
    std::vector<task_t *> *sequence = (std::vector<task_t *> *)p[0];
    mutex_t *mutex = (mutex_t *)p[1];
    task_t *task1 = (task_t *)p[2];
    task_t *task2 = (task_t *)p[3];

    // Lock mutex
    mutex_lock(mutex);

    // Resume can preempt.
    task_resume(task1);
    task_resume(task2);

    // Sequence after possible preemption.
    sequence->push_back(get_current_task());
    task_suspend(CURRENT_TASK_PTR);
}

static void func_resume_2tasks(void *param)
{
    void **p = (void **)param;
    std::vector<task_t *> *sequence = (std::vector<task_t *> *)p[0];
    task_t *task1 = (task_t *)p[1];
    task_t *task2 = (task_t *)p[2];

    // Resume can preempt.
    task_resume(task1);
    task_resume(task2);

    // Sequence after possible preemption.
    sequence->push_back(get_current_task());
    task_suspend(CURRENT_TASK_PTR);
}

static void func_suspend_on_mutex(void *param)
{
    void **p = (void **)param;
    std::vector<task_t *> *sequence = (std::vector<task_t *> *)p[0];
    mutex_t *mutex = (mutex_t *)p[1];

    // Resume can preempt.
    mutex_suspend(mutex);

    // Sequence after possible preemption.
    sequence->push_back(get_current_task());
    task_suspend(CURRENT_TASK_PTR);
}

TEST(
    MutexPriorityInversion,
    HigherPrioritySuspends_OwnerRunning_IncreasePriorityOfOwner)
{
    std::vector<task_t *> actual_sequence;

    initialize_os(this);
    initialize_mutex(this);

    // Lower priority task:
    // - [0] Locks mutex
    // - [1] Resumes higher priority task
    //   - Preemption
    //   - Priority increased
    // - [4] Resumes middle priority task
    //   - Not preempted because of increased priority
    // - [5] Suspends and registers sequence
    void *param1[] = {&actual_sequence, &mtx, &task[2], &task[1]};
    create_one_task(this, 0, &func_lock_mutex_and_resume_2tasks, &param1[0]);

    // Middle priority task:
    // - [6] Suspends and registers sequence
    create_one_task(this, 1, &func, &actual_sequence);

    // Middle priority task:
    // - [2] Suspends on mutex
    //   - Increase mutex owner priority
    // - [3] Suspends and registers sequence
    void *param3[] = {&actual_sequence, &mtx};
    create_one_task(this, 2, &func_suspend_on_mutex, &param3);

    // Given The middle priority task is suspended
    taskX_is_suspended(this, 1);
    // And The higher priority task is suspended
    taskX_is_suspended(this, 2);

    // When The scheduler is called
    call_the_scheduler(this);

    // Then The higher priority task should be scheduled
    // And The lower priority task should be scheduled
    // And The middle priority task should be scheduled
    tasks_should_be_scheduled_in_the_order(
        this, {&task[2], &task[0], &task[1]}, actual_sequence);
}

IGNORE_TEST(
    MutexPriorityInversion,
    HigherPrioritySuspends_OwnerPreempted_IncreasePriorityOfOwner)
{
    std::vector<task_t *> actual_sequence;

    initialize_os(this);
    initialize_mutex(this);

    void *param0[] = {&actual_sequence, &mtx, &task[1]};
    create_one_task(this, 0, &func_lock_mutex_and_resume_1task, &param0[0]);

    void *param1[] = {&actual_sequence, &task[3], &task[2]};
    create_one_task(this, 1, &func_resume_2tasks, &param1[0]);

    create_one_task(this, 2, &func, &actual_sequence);

    void *param3[] = {&actual_sequence, &mtx};
    create_one_task(this, 3, &func_suspend_on_mutex, &param3);

    // Given Task1 is suspended
    taskX_is_suspended(this, 1);
    // And Task2 is suspended
    taskX_is_suspended(this, 2);
    // And Task3 is suspended
    taskX_is_suspended(this, 3);

    // When The scheduler is called
    call_the_scheduler(this);

#if 1
    // Desired

    // Then ...
    // - Task1-3 are suspended
    // - Task0
    //   - Locks mutex
    //   - Resumes Task1
    // - Task1
    //   - Resumes Task3
    // - Task3
    //   - Suspends on mutex
    //   - Registers, suspends and returns
    // - Task1
    //   - Resumes Task2
    //   - Registers, suspends and returns
    // - Task0
    //   - Registers, suspends and returns
    // - Task2
    //   - Registers, suspends and returns

    tasks_should_be_scheduled_in_the_order(
        this, {&task[3], &task[1], &task[0], &task[2]}, actual_sequence);
#else
    // Actual
    // When Task0 Gets its priority increased, the scheduler cannot see it,
    // because Task1 has preempted.
    tasks_should_be_scheduled_in_the_order(
        this, {&task[3], &task[2], &task[1], &task[0]}, actual_sequence);
#endif
}

TEST(
    MutexPriorityInversion,
    HigherPrioritySuspends_OwnerPreempted_RemoveTaskFromReadyList_TODO)
{
    /* While the test
     * HigherPrioritySuspends_OwnerPreempted_IncreasePriorityOfOwner cannot be
     * solved, keep
     * HigherPrioritySuspends_OwnerPreempted_RemoveTaskFromReadyList_TODO.
     * It revealed a bug, which made necessary to create the list of
     * running tasks.
     */
    std::vector<task_t *> actual_sequence;

    initialize_os(this);
    initialize_mutex(this);

    void *param0[] = {&actual_sequence, &mtx, &task[1]};
    create_one_task(this, 0, &func_lock_mutex_and_resume_1task, &param0[0]);

    void *param1[] = {&actual_sequence, &task[3], &task[2]};
    create_one_task(this, 1, &func_resume_2tasks, &param1[0]);

    create_one_task(this, 2, &func, &actual_sequence);

    void *param3[] = {&actual_sequence, &mtx};
    create_one_task(this, 3, &func_suspend_on_mutex, &param3);

    // Given Task1 is suspended
    taskX_is_suspended(this, 1);
    // And Task2 is suspended
    taskX_is_suspended(this, 2);
    // And Task3 is suspended
    taskX_is_suspended(this, 3);

    // When The scheduler is called
    call_the_scheduler(this);

    // Actual
    // When Task0 Gets its priority increased, the scheduler cannot see it,
    // because Task1 has preempted it.

    // Then ...
    // - Task1-3 are suspended
    // - Task0
    //   - Locks mutex
    //   - Resumes Task1
    // - Task1
    //   - Resumes Task3
    // - Task3
    //   - Suspends on mutex
    //   - Registers, suspends and returns
    // - Task1
    //   - Resumes Task2
    // - Task2
    //   - Registers, suspends and returns
    // - Task1
    //   - Registers, suspends and returns
    // - Task0
    //   - Registers, suspends and returns
    tasks_should_be_scheduled_in_the_order(
        this, {&task[3], &task[2], &task[1], &task[0]}, actual_sequence);
}
