/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos.h"

/*
 * Main file: src/semaphore.c
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
    LONGS_EQUAL(0, semaphore_count(&sem));
    LONGS_EQUAL(1, semaphore_max(&sem));

    semaphore_unlock(&sem);
    LONGS_EQUAL(1, semaphore_count(&sem));
    LONGS_EQUAL(1, semaphore_max(&sem));
}

TEST(Sempahore, Counting_GetCount_GetMax)
{
    semaphore_init_locked(&sem, 2);
    LONGS_EQUAL(0, semaphore_count(&sem));
    LONGS_EQUAL(2, semaphore_max(&sem));

    semaphore_unlock(&sem);
    LONGS_EQUAL(1, semaphore_count(&sem));
    LONGS_EQUAL(2, semaphore_max(&sem));

    semaphore_unlock(&sem);
    LONGS_EQUAL(2, semaphore_count(&sem));
    LONGS_EQUAL(2, semaphore_max(&sem));
}
