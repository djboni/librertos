/* Copyright (c) 2016-2023 Djones A. Boni - MIT License */

#define LIBRERTOS_DEBUG_DECLARATIONS
#include "librertos.h"
#include "tests/utils/librertos_custom_tests.h"
#include "tests/utils/librertos_test_utils.h"

/*
 * Main file: src/librertos.c
 * Also compile: tests/mocks/librertos_assert.cpp
 * Also compile: tests/utils/librertos_custom_tests.cpp
 * Also compile: tests/utils/librertos_test_utils.cpp
 */

#include <vector>

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

TEST_GROUP (Mutex) {
    mutex_t mtx;
    task_t task1, task2;

    void setup() {
        mutex_init(&mtx);
    }
    void teardown() {
    }
};

TEST(Mutex, Unlocked_Locks) {
    LONGS_EQUAL(LIBRERTOS_SUCCESS, mutex_lock(&mtx));
}

TEST(Mutex, TryUnlockAnUnlockedMutex_CallsAssertFunction) {
    mock()
        .expectOneCall("librertos_assert")
        .withParameter("msg", "Mutex already unlocked.");

    CHECK_THROWS(AssertionError, mutex_unlock(&mtx));
}

TEST(Mutex, Locked_Unlocks) {
    mutex_lock(&mtx);
    mutex_unlock(&mtx);
}

TEST(Mutex, IsLocked_SameTask) {
    LONGS_EQUAL(0, mutex_is_locked(&mtx));

    mutex_lock(&mtx);
    LONGS_EQUAL(0, mutex_is_locked(&mtx));

    mutex_lock(&mtx);
    LONGS_EQUAL(0, mutex_is_locked(&mtx));
}

TEST(Mutex, IsLocked_OtherTask) {
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

TEST(Mutex, Unlocked_TaskCanLockMultipleTimes) {
    librertos_init();

    scheduler_lock();
    set_current_task(&task1);

    LONGS_EQUAL(LIBRERTOS_SUCCESS, mutex_lock(&mtx));
    LONGS_EQUAL(LIBRERTOS_SUCCESS, mutex_lock(&mtx));
}

TEST(Mutex, Locked_TaskCanUnLockMultipleTimes) {
    librertos_init();

    scheduler_lock();
    set_current_task(&task1);
    mutex_lock(&mtx);
    mutex_lock(&mtx);

    mutex_unlock(&mtx);
    mutex_unlock(&mtx);
}

TEST(Mutex, Unlocked_TaskCanLock_AndUnlock_MultipleTimes) {
    librertos_init();

    scheduler_lock();
    set_current_task(&task1);

    LONGS_EQUAL(LIBRERTOS_SUCCESS, mutex_lock(&mtx));
    mutex_unlock(&mtx);

    LONGS_EQUAL(LIBRERTOS_SUCCESS, mutex_lock(&mtx));
    LONGS_EQUAL(LIBRERTOS_SUCCESS, mutex_lock(&mtx));
    mutex_unlock(&mtx);
    mutex_unlock(&mtx);

    LONGS_EQUAL(LIBRERTOS_SUCCESS, mutex_lock(&mtx));
    LONGS_EQUAL(LIBRERTOS_SUCCESS, mutex_lock(&mtx));
    LONGS_EQUAL(LIBRERTOS_SUCCESS, mutex_lock(&mtx));
    mutex_unlock(&mtx);
    mutex_unlock(&mtx);
    mutex_unlock(&mtx);
}

TEST(Mutex, Locked_SecondTaskCannotLock) {
    librertos_init();

    scheduler_lock();
    set_current_task(&task1);
    mutex_lock(&mtx);
    set_current_task(&task2);

    LONGS_EQUAL(LIBRERTOS_FAIL, mutex_lock(&mtx));
}

TEST(Mutex, Locked_InterruptCannotLock) {
    librertos_init();

    scheduler_lock();
    set_current_task(&task1);
    mutex_lock(&mtx);
    (void)interrupt_lock();

    LONGS_EQUAL(LIBRERTOS_FAIL, mutex_lock(&mtx));
}

TEST(Mutex, Unlocked_InterruptCanLockMultipleTimes) {
    librertos_init();

    (void)interrupt_lock();

    LONGS_EQUAL(LIBRERTOS_SUCCESS, mutex_lock(&mtx));
    mutex_unlock(&mtx);

    LONGS_EQUAL(LIBRERTOS_SUCCESS, mutex_lock(&mtx));
    LONGS_EQUAL(LIBRERTOS_SUCCESS, mutex_lock(&mtx));
    mutex_unlock(&mtx);
    mutex_unlock(&mtx);

    LONGS_EQUAL(LIBRERTOS_SUCCESS, mutex_lock(&mtx));
    LONGS_EQUAL(LIBRERTOS_SUCCESS, mutex_lock(&mtx));
    LONGS_EQUAL(LIBRERTOS_SUCCESS, mutex_lock(&mtx));
    mutex_unlock(&mtx);
    mutex_unlock(&mtx);
    mutex_unlock(&mtx);
}

TEST_GROUP (MutexEvent) {
    char buff[BUFF_SIZE];

    mutex_t mtx;
    task_t task1, task2;
    ParamMutex param1;

    void setup() {
        // Task sequencing buffer
        strcpy(buff, "");

        // Task parameters
        param1 = ParamMutex{&buff[0], &mtx, "A", "L", "U", "_"};

        librertos_init();
        mutex_init(&mtx);
    }
    void teardown() {
    }
};

TEST(MutexEvent, TaskSuspendsOnEvent_ShouldNotBeScheduled) {
    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamMutex::task_sequencing, &param1);

    mutex_lock(&mtx);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("AL_", buff);

    librertos_sched();
    STRCMP_EQUAL("AL_", buff);
}

TEST(MutexEvent, TaskSuspendsOnAvailableEvent_ShouldBeScheduled) {
    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamMutex::task_sequencing, &param1);

    set_current_task(&task1);
    mutex_suspend(&mtx, MAX_DELAY);
    set_current_task(NO_TASK_PTR);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("AU_", buff);
}

TEST(MutexEvent, TaskResumesOnEvent_ShouldBeScheduled) {
    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamMutex::task_sequencing, &param1);

    mutex_lock(&mtx);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("AL_", buff);

    mutex_unlock(&mtx);
    STRCMP_EQUAL("AL_AU_", buff);
}

TEST(MutexEvent, TaskResumesOnEvent_SchedulerLocked_ShouldNotBeScheduled) {
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

TEST(MutexEvent, TaskLockSuspend_UnavailableMutex_ShouldNotBeScheduled) {
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

TEST(MutexEvent, TaskLockSuspend_UnavailableMutex_ShouldBeScheduledWhenUnlocked) {
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

TEST(MutexEvent, TaskLockSuspend_AvailableMutex_ShouldBeScheduled) {
    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamMutex::task_mutex_lock_suspend, &param1);

    librertos_start();
    librertos_sched();

    STRCMP_EQUAL(
        "AU_", // Locks and suspends
        buff);
}

TEST_GROUP (MutexPriorityInversion) {
    mutex_t mtx;
    mutex_t helper;
    task_t task[4];
    uint8_t tasks_used;

    void setup() {
    }
    void teardown() {
    }
};

static void func(void *param) {
    std::vector<task_t *> *sequence = (std::vector<task_t *> *)param;
    sequence->push_back(get_current_task());
    task_suspend(CURRENT_TASK_PTR);
}

TEST(
    MutexPriorityInversion,
    LowerPrioritySuspends_TaskReady_NoChangeInPriorities) {
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
    HigherPrioritySuspends_TaskReady_IncreasePriorityOfOwner) {
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
    HigherPrioritySuspends_TaskUnlocksMutex_OriginalPriorityReturns) {
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
    LowerPrioritySuspends_TaskSuspendedOnEvent_NoChangeInPriorities) {
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
    HigherPrioritySuspends_TaskSuspendedOnEvent_IncreasePriorityOfOwner) {
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

static void func_lock_mutex_and_resume_2tasks(void *param) {
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

static void func_suspend_on_mutex(void *param) {
    void **p = (void **)param;
    std::vector<task_t *> *sequence = (std::vector<task_t *> *)p[0];
    mutex_t *mutex = (mutex_t *)p[1];

    // Resume can preempt.
    mutex_suspend(mutex, MAX_DELAY);

    // Sequence after possible preemption.
    sequence->push_back(get_current_task());
    task_suspend(CURRENT_TASK_PTR);
}

TEST(
    MutexPriorityInversion,
    HigherPrioritySuspends_OwnerRunning_IncreasePriorityOfOwner) {
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

static void func_test2_task0(void *param) {
    void **p = (void **)param;
    auto sequence = (std::vector<int> *)p[0];
    auto mtx = (mutex_t *)p[1];

    // 1. Low priority locks the mutex
    sequence->push_back(1);
    mutex_lock(mtx);

    // 2. Resumes middle priority task
    sequence->push_back(2);
    task_resume(&test.task[1]);

    // 9. Suspend itself
    sequence->push_back(9);
    task_suspend(NULL);

    // 10. Return
    sequence->push_back(10);
}

static void func_test2_task1(void *param) {
    void **p = (void **)param;
    auto sequence = (std::vector<int> *)p[0];
    auto mtx = (mutex_t *)p[1];
    (void)mtx;

    // 3. Resumes high priority task
    sequence->push_back(3);
    task_resume(&test.task[3]);

    // 6. Resumes middle priority task
    sequence->push_back(6);
    task_resume(&test.task[2]);

    // 7. Suspend itself
    sequence->push_back(7);
    task_suspend(NULL);

    // 8. Return
    sequence->push_back(8);
}

static void func_test2_task2(void *param) {
    void **p = (void **)param;
    auto sequence = (std::vector<int> *)p[0];
    auto mtx = (mutex_t *)p[1];
    (void)mtx;

    // 11. Suspend itself
    sequence->push_back(11);
    task_suspend(NULL);

    // 12. Return
    sequence->push_back(12);
}

static void func_test2_task3(void *param) {
    void **p = (void **)param;
    auto sequence = (std::vector<int> *)p[0];
    auto mtx = (mutex_t *)p[1];

    // 4. High priority locks the mutex
    sequence->push_back(4);
    mutex_suspend(mtx, MAX_DELAY);

    // 5. Return
    sequence->push_back(5);
}

TEST(
    MutexPriorityInversion,
    HigherPrioritySuspends_OwnerPreempted_IncreasePriorityOfOwner) {
    std::vector<int> sequence;

    librertos_init();
    mutex_init(&mtx);

    void *param[] = {&sequence, &mtx};

    librertos_create_task(
        LOW_PRIORITY, &test.task[0], &func_test2_task0, &param);
    librertos_create_task(
        LOW_PRIORITY + 1, &test.task[1], &func_test2_task1, &param);
    librertos_create_task(
        HIGH_PRIORITY - 1, &test.task[2], &func_test2_task2, &param);
    librertos_create_task(
        HIGH_PRIORITY, &test.task[3], &func_test2_task3, &param);

    task_suspend(&test.task[1]);
    task_suspend(&test.task[2]);
    task_suspend(&test.task[3]);

    librertos_start();
    librertos_sched();

    std::vector<int> expected{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    CHECK_EQUAL(expected, sequence);
}

static void func_test3_task0(void *param) {
    void **p = (void **)param;
    auto sequence = (std::vector<int> *)p[0];
    auto mtx = (mutex_t *)p[1];

    // 1. Low priority locks the mutex
    sequence->push_back(1);
    mutex_lock(mtx);

    // 2. Suspend itself
    sequence->push_back(2);
    task_suspend(NULL);

    // 3. Resumes middle priority task
    sequence->push_back(3);
    task_resume(&test.task[1]);

    // 14. Suspend itself
    sequence->push_back(14);
    task_suspend(NULL);

    // 15. Return
    sequence->push_back(15);
}

static void func_test3_task1(void *param) {
    void **p = (void **)param;
    auto sequence = (std::vector<int> *)p[0];
    auto mtx = (mutex_t *)p[1];
    (void)mtx;

    // 4. Resumes high priority task
    sequence->push_back(4);
    task_resume(&test.task[3]);

    // 12. Suspend itself
    sequence->push_back(12);
    task_suspend(NULL);

    // 13. Return
    sequence->push_back(13);
}

static void func_test3_task2(void *param) {
    void **p = (void **)param;
    auto sequence = (std::vector<int> *)p[0];
    auto mtx = (mutex_t *)p[1];
    (void)mtx;

    // 8. Suspends and resumes low priority task
    sequence->push_back(8);
    task_suspend(&test.task[0]);

    // 9. Low priority task is already running and should not run here
    sequence->push_back(9);
    task_resume(&test.task[0]);

    // 10. Suspend itself
    sequence->push_back(10);
    task_suspend(NULL);

    // 11. Return
    sequence->push_back(11);
}

static void func_test3_task3(void *param) {
    void **p = (void **)param;
    auto sequence = (std::vector<int> *)p[0];
    auto mtx = (mutex_t *)p[1];

    // 5. High priority locks the mutex
    sequence->push_back(5);
    mutex_suspend(mtx, MAX_DELAY);

    // 6. Resumes middle priority task
    sequence->push_back(6);
    task_resume(&test.task[2]);

    // 7. Return
    sequence->push_back(7);
}

TEST(
    MutexPriorityInversion,
    HigherPrioritySuspends_OwnerSuspendedAndPreempted_DoesNotGetScheduledAgain) {
    std::vector<int> sequence;

    librertos_init();
    mutex_init(&mtx);

    void *param[] = {&sequence, &mtx};

    librertos_create_task(
        LOW_PRIORITY, &test.task[0], &func_test3_task0, &param);
    librertos_create_task(
        LOW_PRIORITY + 1, &test.task[1], &func_test3_task1, &param);
    librertos_create_task(
        HIGH_PRIORITY - 1, &test.task[2], &func_test3_task2, &param);
    librertos_create_task(
        HIGH_PRIORITY, &test.task[3], &func_test3_task3, &param);

    task_suspend(&test.task[1]);
    task_suspend(&test.task[2]);
    task_suspend(&test.task[3]);

    librertos_start();
    librertos_sched();

    std::vector<int> expected{
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    CHECK_EQUAL(expected, sequence);
}

TEST_GROUP (MutexEventNewTest) {
    mutex_t mtx;

    void setup() {
        test_init();
        mutex_init(&mtx);
    }
    void teardown() {
    }
};

TEST(MutexEventNewTest, TaskSuspendsOnEvent_ResumesWithTaskResume) {
    test_create_tasks({0}, NULL, {NULL});
    mutex_lock(&mtx);

    set_current_task(&test.task[0]);
    mutex_suspend(&mtx, MAX_DELAY);

    test_task_is_suspended(&test.task[0]);

    task_resume(&test.task[0]);

    test_task_is_ready(&test.task[0]);
}

TEST(MutexEventNewTest, TaskSuspendsOnEvent_ResumesWithEvent) {
    test_create_tasks({0}, NULL, {NULL});
    mutex_lock(&mtx);

    set_current_task(&test.task[0]);
    mutex_suspend(&mtx, MAX_DELAY);

    test_task_is_suspended(&test.task[0]);

    mutex_unlock(&mtx);

    test_task_is_ready(&test.task[0]);
}

TEST(MutexEventNewTest, TaskDelaysOnEvent_ResumesWithTaskResume) {
    test_create_tasks({0}, NULL, {NULL});
    mutex_lock(&mtx);

    set_current_task(&test.task[0]);
    mutex_suspend(&mtx, 1);

    test_task_is_delayed_current(&test.task[0]);

    task_resume(&test.task[0]);

    test_task_is_ready(&test.task[0]);
}

TEST(MutexEventNewTest, TaskDelaysOnEvent_ResumesWithEvent) {
    test_create_tasks({0}, NULL, {NULL});
    mutex_lock(&mtx);

    set_current_task(&test.task[0]);
    mutex_suspend(&mtx, 1);

    test_task_is_delayed_current(&test.task[0]);

    mutex_unlock(&mtx);

    test_task_is_ready(&test.task[0]);
}

TEST(MutexEventNewTest, TaskDelaysOnEvent_ResumesWithTickInterrupt) {
    test_create_tasks({0}, NULL, {NULL});
    mutex_lock(&mtx);

    set_current_task(&test.task[0]);
    mutex_suspend(&mtx, 1);

    test_task_is_delayed_current(&test.task[0]);

    librertos_tick_interrupt();

    test_task_is_ready(&test.task[0]);
}

TEST(MutexEventNewTest, TaskLockSuspendOnEvent_Success) {
    test_create_tasks({0}, NULL, {NULL});

    set_current_task(&test.task[0]);

    LONGS_EQUAL(1, mutex_lock_suspend(&mtx, MAX_DELAY));
    test_task_is_ready(&test.task[0]);
}

TEST(MutexEventNewTest, TaskLockSuspendOnEvent_Fail) {
    test_create_tasks({0}, NULL, {NULL});
    mutex_lock(&mtx);

    set_current_task(&test.task[0]);

    LONGS_EQUAL(0, mutex_lock_suspend(&mtx, MAX_DELAY));
    test_task_is_suspended(&test.task[0]);
}

TEST(MutexEventNewTest, TaskLockDelaysOnEvent_Fail) {
    test_create_tasks({0}, NULL, {NULL});
    mutex_lock(&mtx);

    set_current_task(&test.task[0]);

    LONGS_EQUAL(0, mutex_lock_suspend(&mtx, 1));
    test_task_is_delayed_current(&test.task[0]);
}
