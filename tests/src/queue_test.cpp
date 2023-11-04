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

#include <cstring>

typedef struct
{
    uint32_t x;
} que_struct;

static const uint8_t que_guards = 2;
static const que_struct que_struct_zero = {0x00000000};
static const que_struct que_struct_val_A = {0x5B5B5B5B};
static const que_struct que_struct_val_B = {0x5C5C5C5C};
static const que_struct que_struct_val_C = {0x5D5D5D5D};
static const que_struct que_struct_val_D = {0x5E5E5E5E};

static void initialize_buff(que_struct *buff, uint8_t size) {
    // First and last elements of buff are guards
    memset(buff, 0x5A, size * sizeof(que_struct));
}

static void finalize_buff(que_struct *buff, uint8_t size) {
    que_struct expected;

    // Initialize expected and validate both guards
    memset(&expected, 0x5A, sizeof(expected));
    MEMCMP_EQUAL(&expected, &buff[0], sizeof(que_struct));
    MEMCMP_EQUAL(&expected, &buff[size - 1], sizeof(que_struct));
}

TEST_GROUP (Queue_Size1) {
    static const uint8_t que_size = 1;
    static const uint8_t buff_size = que_size + que_guards;

    que_struct buff[buff_size];
    queue_t que;

    void setup() {
        initialize_buff(&buff[0], buff_size);
        queue_init(&que, &buff[1], que_size, sizeof(que_struct));
    }

    void teardown() {
        finalize_buff(&buff[0], buff_size);
    }
};

TEST(Queue_Size1, TwoBuffGuards) {
    // We literally want two guards (que_guards == 2)
    LONGS_EQUAL(2, que_guards);
}

TEST(Queue_Size1, Empty_CannotRead) {
    que_struct data;
    LONGS_EQUAL(FAIL, queue_read(&que, &data));
}

TEST(Queue_Size1, Empty_Writes) {
    LONGS_EQUAL(SUCCESS, queue_write(&que, &que_struct_val_A));
}

TEST(Queue_Size1, Empty_CannotWriteTwice) {
    queue_write(&que, &que_struct_val_A);
    LONGS_EQUAL(FAIL, queue_write(&que, &que_struct_val_B));
}

TEST(Queue_Size1, NotEmpty_Reads) {
    que_struct data = que_struct_zero;
    queue_write(&que, &que_struct_val_A);

    LONGS_EQUAL(SUCCESS, queue_read(&que, &data));
    MEMCMP_EQUAL(&que_struct_val_A, &data, sizeof(que_struct));
}

TEST(Queue_Size1, WrapsHead_WrapsTail) {
    que_struct data = que_struct_zero;
    queue_write(&que, &que_struct_val_A);
    queue_read(&que, &data);

    // Write wraps head
    LONGS_EQUAL(SUCCESS, queue_write(&que, &que_struct_val_B));

    // Read wraps tail
    LONGS_EQUAL(SUCCESS, queue_read(&que, &data));
    MEMCMP_EQUAL(&que_struct_val_B, &data, sizeof(que_struct));
}

TEST(Queue_Size1, NumFree_NumUsed_Empty_Full_NumItems_ItemSize) {
    LONGS_EQUAL(1, queue_get_num_free(&que));
    LONGS_EQUAL(0, queue_get_num_used(&que));
    LONGS_EQUAL(1, queue_is_empty(&que));
    LONGS_EQUAL(0, queue_is_full(&que));
    LONGS_EQUAL(1, queue_get_num_items(&que));
    LONGS_EQUAL(sizeof(que_struct), queue_get_item_size(&que));

    queue_write(&que, &que_struct_val_A);

    LONGS_EQUAL(0, queue_get_num_free(&que));
    LONGS_EQUAL(1, queue_get_num_used(&que));
    LONGS_EQUAL(0, queue_is_empty(&que));
    LONGS_EQUAL(1, queue_is_full(&que));
    LONGS_EQUAL(1, queue_get_num_items(&que));
    LONGS_EQUAL(sizeof(que_struct), queue_get_item_size(&que));
}

TEST_GROUP (Queue_Size2) {
    static const uint8_t que_size = 2;
    static const uint8_t buff_size = que_size + que_guards;

    que_struct buff[buff_size];
    queue_t que;

    void setup() {
        initialize_buff(&buff[0], buff_size);
        queue_init(&que, &buff[1], que_size, sizeof(que_struct));
    }

    void teardown() {
        finalize_buff(&buff[0], buff_size);
    }
};

TEST(Queue_Size2, Empty_CannotRead) {
    que_struct data;
    LONGS_EQUAL(FAIL, queue_read(&que, &data));
}

TEST(Queue_Size2, Empty_Writes) {
    LONGS_EQUAL(SUCCESS, queue_write(&que, &que_struct_val_A));
}
TEST(Queue_Size2, Empty_WritesTwice) {
    queue_write(&que, &que_struct_val_A);
    LONGS_EQUAL(SUCCESS, queue_write(&que, &que_struct_val_B));
}

TEST(Queue_Size2, Empty_CannotWriteThreeTimes) {
    queue_write(&que, &que_struct_val_A);
    queue_write(&que, &que_struct_val_B);
    LONGS_EQUAL(FAIL, queue_write(&que, &que_struct_val_C));
}

TEST(Queue_Size2, NotEmpty_Reads) {
    que_struct data = que_struct_zero;
    queue_write(&que, &que_struct_val_A);

    LONGS_EQUAL(SUCCESS, queue_read(&que, &data));
    MEMCMP_EQUAL(&que_struct_val_A, &data, sizeof(que_struct));
}

TEST(Queue_Size2, FirstInFirstOut) {
    que_struct data = que_struct_zero;
    queue_write(&que, &que_struct_val_A);
    queue_write(&que, &que_struct_val_B);

    LONGS_EQUAL(SUCCESS, queue_read(&que, &data));
    MEMCMP_EQUAL(&que_struct_val_A, &data, sizeof(que_struct));
}

TEST(Queue_Size2, FirstInFirstOut_2) {
    que_struct data = que_struct_zero;
    queue_write(&que, &que_struct_val_A);
    queue_write(&que, &que_struct_val_B);
    queue_read(&que, &data);

    LONGS_EQUAL(SUCCESS, queue_read(&que, &data));
    MEMCMP_EQUAL(&que_struct_val_B, &data, sizeof(que_struct));
}

TEST(Queue_Size2, WrapsHead_WrapsTail) {
    que_struct data = que_struct_zero;
    queue_write(&que, &que_struct_val_A);
    queue_write(&que, &que_struct_val_B);
    queue_read(&que, &data);

    // Write wraps head
    LONGS_EQUAL(SUCCESS, queue_write(&que, &que_struct_val_C));

    queue_read(&que, &data);

    // Read wraps tail
    LONGS_EQUAL(SUCCESS, queue_read(&que, &data));
    MEMCMP_EQUAL(&que_struct_val_C, &data, sizeof(que_struct));
}

TEST(Queue_Size2, NumFree_NumUsed_Empty_Full_NumItems_ItemSize) {
    LONGS_EQUAL(2, queue_get_num_free(&que));
    LONGS_EQUAL(0, queue_get_num_used(&que));
    LONGS_EQUAL(1, queue_is_empty(&que));
    LONGS_EQUAL(0, queue_is_full(&que));
    LONGS_EQUAL(2, queue_get_num_items(&que));
    LONGS_EQUAL(sizeof(que_struct), queue_get_item_size(&que));

    queue_write(&que, &que_struct_val_A);

    LONGS_EQUAL(1, queue_get_num_free(&que));
    LONGS_EQUAL(1, queue_get_num_used(&que));
    LONGS_EQUAL(0, queue_is_empty(&que));
    LONGS_EQUAL(0, queue_is_full(&que));
    LONGS_EQUAL(2, queue_get_num_items(&que));
    LONGS_EQUAL(sizeof(que_struct), queue_get_item_size(&que));

    queue_write(&que, &que_struct_val_B);

    LONGS_EQUAL(0, queue_get_num_free(&que));
    LONGS_EQUAL(2, queue_get_num_used(&que));
    LONGS_EQUAL(0, queue_is_empty(&que));
    LONGS_EQUAL(1, queue_is_full(&que));
    LONGS_EQUAL(2, queue_get_num_items(&que));
    LONGS_EQUAL(sizeof(que_struct), queue_get_item_size(&que));
}

TEST_GROUP (Queue_Size0) {
    static const uint8_t que_size = 0;
    static const uint8_t buff_size = que_size + que_guards;

    que_struct buff[buff_size];
    queue_t que;

    void setup() {
        initialize_buff(&buff[0], buff_size);
        queue_init(&que, &buff[1], que_size, sizeof(que_struct));
    }

    void teardown() {
        finalize_buff(&buff[0], buff_size);
    }
};

TEST(Queue_Size0, Empty_CannotRead) {
    que_struct data;
    LONGS_EQUAL(FAIL, queue_read(&que, &data));
}

TEST(Queue_Size0, Full_CannotWrite) {
    LONGS_EQUAL(FAIL, queue_write(&que, &que_struct_val_A));
}

TEST(Queue_Size0, NumFree_NumUsed_Empty_Full_NumItems_ItemSize) {
    LONGS_EQUAL(0, queue_get_num_free(&que));
    LONGS_EQUAL(0, queue_get_num_used(&que));
    LONGS_EQUAL(1, queue_is_empty(&que));
    LONGS_EQUAL(1, queue_is_full(&que));
    LONGS_EQUAL(0, queue_get_num_items(&que));
    LONGS_EQUAL(sizeof(que_struct), queue_get_item_size(&que));
}

TEST_GROUP (QueueEvent) {
    char buff[BUFF_SIZE];

    queue_t que;
    int8_t que_buff[1];
    task_t task1, task2;
    ParamQueue param1;

    void setup() {
        // Task sequencing buffer
        strcpy(buff, "");

        // Task parameters
        param1 = ParamQueue{&buff[0], &que, "A", "L", "U", "_"};

        // Initialize
        librertos_init();
        queue_init(
            &que,
            &que_buff[0],
            sizeof(que_buff) / sizeof(que_buff[0]),
            sizeof(que_buff[0]));
    }
    void teardown() {
    }
};

TEST(QueueEvent, TaskSuspendsOnEvent_ShouldNotBeScheduled) {
    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamQueue::task_sequencing, &param1);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("AL_", buff);

    librertos_sched();
    STRCMP_EQUAL("AL_", buff);
}

TEST(QueueEvent, TaskSuspendsOnAvailableEvent_ShouldBeScheduled) {
    int8_t data = 0;

    queue_write(&que, &data);

    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamQueue::task_sequencing, &param1);

    set_current_task(&task1);
    queue_suspend(&que, MAX_DELAY);
    set_current_task(NO_TASK_PTR);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("AU_", buff);
}

TEST(QueueEvent, TaskResumesOnEvent_ShouldBeScheduled) {
    int8_t data = 0;

    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamQueue::task_sequencing, &param1);

    librertos_start();
    librertos_sched();
    STRCMP_EQUAL("AL_", buff);

    queue_write(&que, &data);
    STRCMP_EQUAL("AL_AU_", buff);
}

TEST(QueueEvent, TaskResumesOnEvent_SchedulerLocked_ShouldNotBeScheduled) {
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

TEST(QueueEvent, TaskReadSuspend_EmptyQueue_ShouldNotBeScheduled) {
    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamQueue::task_queue_read_suspend, &param1);

    librertos_start();

    librertos_sched();

    STRCMP_EQUAL(
        "AL_", // Suspends
        buff);
}

TEST(QueueEvent, TaskReadSuspend_EmptyQueue_ShouldBeScheduledWhenWritten) {
    int8_t data = 0;

    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamQueue::task_queue_read_suspend, &param1);

    librertos_start();

    librertos_sched();

    STRCMP_EQUAL(
        "AL_", // Suspends
        buff);

    queue_write(&que, &data);

    STRCMP_EQUAL(
        "AL_"  // Suspends (previous)
        "AU_"  // Reads
        "AL_", // Suspends
        buff);
}

TEST(QueueEvent, TaskReadSuspend_NonEmptyQueue_ShouldBeScheduled) {
    int8_t data = 0;

    librertos_create_task(
        LOW_PRIORITY, &task1, &ParamQueue::task_queue_read_suspend, &param1);

    queue_write(&que, &data);

    librertos_start();
    librertos_sched();

    STRCMP_EQUAL(
        "AU_"  // Reads
        "AL_", // Suspends
        buff);
}

TEST_GROUP (QueueEventNewTest) {
    uint8_t que_buff[2];
    queue_t que;

    void setup() {
        test_init();
        queue_init(
            &que,
            &que_buff[0],
            sizeof(que_buff) / sizeof(que_buff[0]),
            sizeof(que_buff[0]));
    }
    void teardown() {
    }
};

TEST(QueueEventNewTest, TaskSuspendsOnEvent_ResumesWithTaskResume) {
    test_create_tasks({0}, NULL, {NULL});

    set_current_task(&test.task[0]);
    queue_suspend(&que, MAX_DELAY);

    test_task_is_suspended(&test.task[0]);

    task_resume(&test.task[0]);

    test_task_is_ready(&test.task[0]);
}

TEST(QueueEventNewTest, TaskSuspendsOnEvent_ResumesWithEvent) {
    uint8_t data = 0;

    test_create_tasks({0}, NULL, {NULL});

    set_current_task(&test.task[0]);
    queue_suspend(&que, MAX_DELAY);

    test_task_is_suspended(&test.task[0]);

    queue_write(&que, &data);

    test_task_is_ready(&test.task[0]);
}

TEST(QueueEventNewTest, TaskDelaysOnEvent_ResumesWithTaskResume) {
    test_create_tasks({0}, NULL, {NULL});

    set_current_task(&test.task[0]);
    queue_suspend(&que, 1);

    test_task_is_delayed_current(&test.task[0]);

    task_resume(&test.task[0]);

    test_task_is_ready(&test.task[0]);
}

TEST(QueueEventNewTest, TaskDelaysOnEvent_ResumesWithEvent) {
    uint8_t data = 0;

    test_create_tasks({0}, NULL, {NULL});

    set_current_task(&test.task[0]);
    queue_suspend(&que, 1);

    test_task_is_delayed_current(&test.task[0]);

    queue_write(&que, &data);

    test_task_is_ready(&test.task[0]);
}

TEST(QueueEventNewTest, TaskDelaysOnEvent_ResumesWithTickInterrupt) {
    test_create_tasks({0}, NULL, {NULL});

    set_current_task(&test.task[0]);
    queue_suspend(&que, 1);

    test_task_is_delayed_current(&test.task[0]);

    librertos_tick_interrupt();

    test_task_is_ready(&test.task[0]);
}

TEST(QueueEventNewTest, TaskReadSuspendOnEvent_Success) {
    uint8_t data_w = 0x5A;
    uint8_t data_r = 0xA5;

    test_create_tasks({0}, NULL, {NULL});
    queue_write(&que, &data_w);

    set_current_task(&test.task[0]);

    LONGS_EQUAL(1, queue_read_suspend(&que, &data_r, MAX_DELAY));
    LONGS_EQUAL(0x5A, data_r);
    test_task_is_ready(&test.task[0]);
}

TEST(QueueEventNewTest, TaskReadSuspendOnEvent_Fail) {
    uint8_t data_r = 0xA5;

    test_create_tasks({0}, NULL, {NULL});

    set_current_task(&test.task[0]);

    LONGS_EQUAL(0, queue_read_suspend(&que, &data_r, MAX_DELAY));
    LONGS_EQUAL(0xA5, data_r);
    test_task_is_suspended(&test.task[0]);
}

TEST(QueueEventNewTest, TaskReadDelaysOnEvent_Fail) {
    uint8_t data_r = 0xA5;

    test_create_tasks({0}, NULL, {NULL});

    set_current_task(&test.task[0]);

    LONGS_EQUAL(0, queue_read_suspend(&que, &data_r, 1));
    LONGS_EQUAL(0xA5, data_r);
    test_task_is_delayed_current(&test.task[0]);
}
