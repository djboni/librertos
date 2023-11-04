#include "librertos_test_utils.h"
#include "librertos_impl.h"

int8_t kernel_mode = LIBRERTOS_PREEMPTIVE;

void time_travel(tick_t ticks_to_the_future) {
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        librertos.tick += ticks_to_the_future;
    }
    CRITICAL_EXIT();
}

void set_tick(tick_t tick) {
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        librertos.tick = tick;
    }
    CRITICAL_EXIT();
}
