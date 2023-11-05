/* Copyright (c) 2016-2023 Djones A. Boni - MIT License */

#include "librertos.h"

#include "stm32f1xx_hal.h"

void HAL_IncTick(void) {
    uwTick += uwTickFreq;
    librertos_tick_interrupt();
}
