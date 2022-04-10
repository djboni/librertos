/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos_impl.h"

librertos_t librertos;

void librertos_init(void)
{
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        librertos.tick = 0;
    }
    CRITICAL_EXIT();
}

void tick_interrupt(void)
{
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        librertos.tick++;
    }
    CRITICAL_EXIT();
}

tick_t get_tick(void)
{
    tick_t tick;

    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        tick = librertos.tick;
    }
    CRITICAL_EXIT();

    return tick;
}
