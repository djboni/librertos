/* Copyright (c) 2016-2023 Djones A. Boni - MIT License */

#include "librertos.h"

#include "stm32f1xx_hal.h"

void SysTick_Handler(void) {
    task_t *interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    NVIC_ClearPendingIRQ(SysTick_IRQn);
    interrupt_unlock(interrupted_task);
}
