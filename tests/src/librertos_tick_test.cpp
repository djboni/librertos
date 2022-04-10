/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos.h"
#include "librertos_impl.h"

/*
 * Main file: src/librertos.c
 * Also compile: tests/utils/librertos_test_utils.c
 */

#include "tests/utils/librertos_test_utils.h"

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

TEST_GROUP (TimeKeeper)
{
    void setup() { librertos_init(); }
    void teardown() {}
};

TEST(TimeKeeper, InitialTickIsZero)
{
    LONGS_EQUAL(0, get_tick());
}

TEST(TimeKeeper, InterruptIncrementsTick)
{
    tick_interrupt();
    LONGS_EQUAL(1, get_tick());
}

TEST(TimeKeeper, InterruptIncrementsTick_2)
{
    tick_interrupt();
    tick_interrupt();
    LONGS_EQUAL(2, get_tick());
}

TEST(TimeKeeper, TimeTravelAddsToTick)
{
    time_travel(100);
    LONGS_EQUAL(100, get_tick());
}

TEST(TimeKeeper, TimeTravelAddsToTick_2)
{
    tick_interrupt();
    time_travel(100);
    LONGS_EQUAL(101, get_tick());
}

TEST(TimeKeeper, SetTick)
{
    set_tick(100);
    LONGS_EQUAL(100, get_tick());
}

TEST(TimeKeeper, SetTick_2)
{
    tick_interrupt();
    set_tick(100);
    LONGS_EQUAL(100, get_tick());
}
