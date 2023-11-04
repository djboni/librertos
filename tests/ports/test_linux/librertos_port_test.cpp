/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos_port.h"

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

TEST_GROUP (TestInterruptsBalanced)
{
    void setup() {}
    void teardown() {}
};

TEST(TestInterruptsBalanced, Construct_OK)
{
    InterruptsBalanced balanced;
}

TEST(TestInterruptsBalanced, IncrementAndDecrement_OK)
{
    InterruptsBalanced balanced;

    ++balanced;
    --balanced;
}

TEST(TestInterruptsBalanced, IncrementAndDecrement_OK_2)
{
    InterruptsBalanced balanced;

    ++balanced;
    --balanced;

    ++balanced;
    --balanced;
}

static void should_throw_on_destructor(bool &throws_after_increment)
{
    InterruptsBalanced balanced;
    ++balanced;
    throws_after_increment = true;
}

TEST(TestInterruptsBalanced, JustIncrement_Throws)
{
    bool throws_after_increment = false;

    CHECK_THROWS(
        InterruptsBalanced::Imbalanced,
        should_throw_on_destructor(throws_after_increment));

    CHECK_TRUE(throws_after_increment);
}

TEST(TestInterruptsBalanced, JustDecrement_Throws)
{
    InterruptsBalanced balanced;

    CHECK_THROWS(InterruptsBalanced::Imbalanced, --balanced);
}
