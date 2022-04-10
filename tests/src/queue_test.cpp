/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos.h"

/*
 * Main file: src/queue.c
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

static void initialize_buff(que_struct *buff, uint8_t size)
{
    // First and last elements of buff are guards
    memset(buff, 0x5A, size * sizeof(que_struct));
}

static void finalize_buff(que_struct *buff, uint8_t size)
{
    que_struct expected;

    // Initialize expected and validate both guards
    memset(&expected, 0x5A, sizeof(expected));
    MEMCMP_EQUAL(&expected, &buff[0], sizeof(que_struct));
    MEMCMP_EQUAL(&expected, &buff[size - 1], sizeof(que_struct));
}

TEST_GROUP (Queue_Size1)
{
    static const uint8_t que_size = 1;
    static const uint8_t buff_size = que_size + que_guards;

    que_struct buff[buff_size];
    queue_t que;

    void setup()
    {
        initialize_buff(&buff[0], buff_size);
        queue_init(&que, &buff[1], que_size, sizeof(que_struct));
    }

    void teardown() { finalize_buff(&buff[0], buff_size); }
};

TEST(Queue_Size1, TwoBuffGuards)
{
    // We literally want two guards (que_guards == 2)
    LONGS_EQUAL(2, que_guards);
}

TEST(Queue_Size1, Empty_CannotRead)
{
    que_struct data;
    LONGS_EQUAL(FAIL, queue_read(&que, &data));
}

TEST(Queue_Size1, Empty_Writes)
{
    LONGS_EQUAL(SUCCESS, queue_write(&que, &que_struct_val_A));
}

TEST(Queue_Size1, Empty_CannotWriteTwice)
{
    queue_write(&que, &que_struct_val_A);
    LONGS_EQUAL(FAIL, queue_write(&que, &que_struct_val_B));
}

TEST(Queue_Size1, NotEmpty_Reads)
{
    que_struct data = que_struct_zero;
    queue_write(&que, &que_struct_val_A);

    LONGS_EQUAL(SUCCESS, queue_read(&que, &data));
    MEMCMP_EQUAL(&que_struct_val_A, &data, sizeof(que_struct));
}

TEST(Queue_Size1, WrapsHead_WrapsTail)
{
    que_struct data = que_struct_zero;
    queue_write(&que, &que_struct_val_A);
    queue_read(&que, &data);

    // Write wraps head
    LONGS_EQUAL(SUCCESS, queue_write(&que, &que_struct_val_B));

    // Read wraps tail
    LONGS_EQUAL(SUCCESS, queue_read(&que, &data));
    MEMCMP_EQUAL(&que_struct_val_B, &data, sizeof(que_struct));
}

TEST(Queue_Size1, NumFree_NumUsed_Empty_Full_NumItems_ItemSize)
{
    LONGS_EQUAL(1, queue_numfree(&que));
    LONGS_EQUAL(0, queue_numused(&que));
    LONGS_EQUAL(1, queue_isempty(&que));
    LONGS_EQUAL(0, queue_isfull(&que));
    LONGS_EQUAL(1, queue_numitems(&que));
    LONGS_EQUAL(sizeof(que_struct), queue_itemsize(&que));

    queue_write(&que, &que_struct_val_A);

    LONGS_EQUAL(0, queue_numfree(&que));
    LONGS_EQUAL(1, queue_numused(&que));
    LONGS_EQUAL(0, queue_isempty(&que));
    LONGS_EQUAL(1, queue_isfull(&que));
    LONGS_EQUAL(1, queue_numitems(&que));
    LONGS_EQUAL(sizeof(que_struct), queue_itemsize(&que));
}

TEST_GROUP (Queue_Size2)
{
    static const uint8_t que_size = 2;
    static const uint8_t buff_size = que_size + que_guards;

    que_struct buff[buff_size];
    queue_t que;

    void setup()
    {
        initialize_buff(&buff[0], buff_size);
        queue_init(&que, &buff[1], que_size, sizeof(que_struct));
    }

    void teardown() { finalize_buff(&buff[0], buff_size); }
};

TEST(Queue_Size2, Empty_CannotRead)
{
    que_struct data;
    LONGS_EQUAL(FAIL, queue_read(&que, &data));
}

TEST(Queue_Size2, Empty_Writes)
{
    LONGS_EQUAL(SUCCESS, queue_write(&que, &que_struct_val_A));
}
TEST(Queue_Size2, Empty_WritesTwice)
{
    queue_write(&que, &que_struct_val_A);
    LONGS_EQUAL(SUCCESS, queue_write(&que, &que_struct_val_B));
}

TEST(Queue_Size2, Empty_CannotWriteThreeTimes)
{
    queue_write(&que, &que_struct_val_A);
    queue_write(&que, &que_struct_val_B);
    LONGS_EQUAL(FAIL, queue_write(&que, &que_struct_val_C));
}

TEST(Queue_Size2, NotEmpty_Reads)
{
    que_struct data = que_struct_zero;
    queue_write(&que, &que_struct_val_A);

    LONGS_EQUAL(SUCCESS, queue_read(&que, &data));
    MEMCMP_EQUAL(&que_struct_val_A, &data, sizeof(que_struct));
}

TEST(Queue_Size2, FirstInFirstOut)
{
    que_struct data = que_struct_zero;
    queue_write(&que, &que_struct_val_A);
    queue_write(&que, &que_struct_val_B);

    LONGS_EQUAL(SUCCESS, queue_read(&que, &data));
    MEMCMP_EQUAL(&que_struct_val_A, &data, sizeof(que_struct));
}

TEST(Queue_Size2, FirstInFirstOut_2)
{
    que_struct data = que_struct_zero;
    queue_write(&que, &que_struct_val_A);
    queue_write(&que, &que_struct_val_B);
    queue_read(&que, &data);

    LONGS_EQUAL(SUCCESS, queue_read(&que, &data));
    MEMCMP_EQUAL(&que_struct_val_B, &data, sizeof(que_struct));
}

TEST(Queue_Size2, WrapsHead_WrapsTail)
{
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

TEST(Queue_Size2, NumFree_NumUsed_Empty_Full_NumItems_ItemSize)
{
    LONGS_EQUAL(2, queue_numfree(&que));
    LONGS_EQUAL(0, queue_numused(&que));
    LONGS_EQUAL(1, queue_isempty(&que));
    LONGS_EQUAL(0, queue_isfull(&que));
    LONGS_EQUAL(2, queue_numitems(&que));
    LONGS_EQUAL(sizeof(que_struct), queue_itemsize(&que));

    queue_write(&que, &que_struct_val_A);

    LONGS_EQUAL(1, queue_numfree(&que));
    LONGS_EQUAL(1, queue_numused(&que));
    LONGS_EQUAL(0, queue_isempty(&que));
    LONGS_EQUAL(0, queue_isfull(&que));
    LONGS_EQUAL(2, queue_numitems(&que));
    LONGS_EQUAL(sizeof(que_struct), queue_itemsize(&que));

    queue_write(&que, &que_struct_val_B);

    LONGS_EQUAL(0, queue_numfree(&que));
    LONGS_EQUAL(2, queue_numused(&que));
    LONGS_EQUAL(0, queue_isempty(&que));
    LONGS_EQUAL(1, queue_isfull(&que));
    LONGS_EQUAL(2, queue_numitems(&que));
    LONGS_EQUAL(sizeof(que_struct), queue_itemsize(&que));
}

TEST_GROUP (Queue_Size0)
{
    static const uint8_t que_size = 0;
    static const uint8_t buff_size = que_size + que_guards;

    que_struct buff[buff_size];
    queue_t que;

    void setup()
    {
        initialize_buff(&buff[0], buff_size);
        queue_init(&que, &buff[1], que_size, sizeof(que_struct));
    }

    void teardown() { finalize_buff(&buff[0], buff_size); }
};

TEST(Queue_Size0, Empty_CannotRead)
{
    que_struct data;
    LONGS_EQUAL(FAIL, queue_read(&que, &data));
}

TEST(Queue_Size0, Full_CannotWrite)
{
    LONGS_EQUAL(FAIL, queue_write(&que, &que_struct_val_A));
}

TEST(Queue_Size0, NumFree_NumUsed_Empty_Full_NumItems_ItemSize)
{
    LONGS_EQUAL(0, queue_numfree(&que));
    LONGS_EQUAL(0, queue_numused(&que));
    LONGS_EQUAL(1, queue_isempty(&que));
    LONGS_EQUAL(1, queue_isfull(&que));
    LONGS_EQUAL(0, queue_numitems(&que));
    LONGS_EQUAL(sizeof(que_struct), queue_itemsize(&que));
}
