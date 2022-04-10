/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos.h"

enum
{
    MUTEX_UNLOCKED = 0,
    MUTEX_LOCKED = 1
};

void mutex_init(mutex_t *mtx)
{
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        mtx->locked = 0;
    }
    CRITICAL_EXIT();
}

result_t mutex_lock(mutex_t *mtx)
{
    result_t result = FAIL;
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        if (mtx->locked == MUTEX_UNLOCKED)
        {
            mtx->locked = MUTEX_LOCKED;
            result = SUCCESS;
        }
    }
    CRITICAL_EXIT();

    return result;
}

result_t mutex_unlock(mutex_t *mtx)
{
    result_t result = FAIL;
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        if (mtx->locked != MUTEX_UNLOCKED)
        {
            mtx->locked = MUTEX_UNLOCKED;
            result = SUCCESS;
        }
    }
    CRITICAL_EXIT();

    return result;
}

uint8_t mutex_islocked(mutex_t *mtx)
{
    uint8_t locked;
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        locked = mtx->locked;
    }
    CRITICAL_EXIT();

    return locked;
}
