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

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

TEST_GROUP (TimeKeeper) {
    void setup() {
        librertos_init();
    }
    void teardown() {
    }
};

TEST(TimeKeeper, InitialTickIsZero) {
    LONGS_EQUAL(0, get_tick());
}

TEST(TimeKeeper, InterruptIncrementsTick) {
    librertos_tick_interrupt();
    LONGS_EQUAL(1, get_tick());
}

TEST(TimeKeeper, InterruptIncrementsTick_2) {
    librertos_tick_interrupt();
    librertos_tick_interrupt();
    LONGS_EQUAL(2, get_tick());
}

TEST(TimeKeeper, TimeTravelAddsToTick) {
    time_travel(100);
    LONGS_EQUAL(100, get_tick());
}

TEST(TimeKeeper, TimeTravelAddsToTick_2) {
    librertos_tick_interrupt();
    time_travel(100);
    LONGS_EQUAL(101, get_tick());
}

TEST(TimeKeeper, SetTick) {
    set_tick(100);
    LONGS_EQUAL(100, get_tick());
}

TEST(TimeKeeper, SetTick_2) {
    librertos_tick_interrupt();
    set_tick(100);
    LONGS_EQUAL(100, get_tick());
}

TEST_GROUP (PeriodicMacro) {
    void setup() {
        librertos_init();
    }
    void teardown() {
    }
};

TEST(PeriodicMacro, ShouldNotRunOnTick0) {
    int var = 0;
    auto increment_periodically_10ticks = [](int &v) { PERIODIC_BLOCK(10) { v++; }; };

    increment_periodically_10ticks(var);

    LONGS_EQUAL(0, var);
}

TEST(PeriodicMacro, ShouldNotRunBeforePeriod) {
    int var = 0;
    auto increment_periodically_10ticks = [](int &v) { PERIODIC_BLOCK(10) { v++; }; };

    increment_periodically_10ticks(var);

    time_travel(9);
    increment_periodically_10ticks(var);

    LONGS_EQUAL(0, var);
}

TEST(PeriodicMacro, ShouldRunAfterPeriod) {
    int var = 0;
    auto increment_periodically_10ticks = [](int &v) { PERIODIC_BLOCK(10) { v++; }; };

    time_travel(10);
    increment_periodically_10ticks(var);

    LONGS_EQUAL(1, var);
}

TEST(PeriodicMacro, ShouldRunOnceAfterPeriod) {
    int var = 0;
    auto increment_periodically_10ticks = [](int &v) { PERIODIC_BLOCK(10) { v++; }; };

    time_travel(10);
    increment_periodically_10ticks(var);
    increment_periodically_10ticks(var);

    LONGS_EQUAL(1, var);
}

TEST(PeriodicMacro, ShouldNotRunBeforePeriod_2) {
    int var = 0;
    auto increment_periodically_10ticks = [](int &v) { PERIODIC_BLOCK(10) { v++; }; };

    time_travel(10);
    increment_periodically_10ticks(var);

    time_travel(9);
    increment_periodically_10ticks(var);

    LONGS_EQUAL(1, var);
}

TEST(PeriodicMacro, ShouldRunAfterPeriod_2) {
    int var = 0;
    auto increment_periodically_10ticks = [](int &v) { PERIODIC_BLOCK(10) { v++; }; };

    time_travel(10);
    increment_periodically_10ticks(var);

    time_travel(10);
    increment_periodically_10ticks(var);

    LONGS_EQUAL(2, var);
}
