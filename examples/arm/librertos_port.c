/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos.h"

#include "stm32f1xx_hal.h"
// #include "stm32f1xx_hal_gpio.h"

void led_config(void) {
    /* Configuration handled on hardware initialization. */
    led_off();
}

void led_on(void) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

void led_off(void) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
}

void led_toggle(void) {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
}

void port_init(void) {
    led_config();
}

void idle_wait_interrupt(void) {
}

void port_enable_tick_interrupt(void) {
    /* Configuration handled on hardware initialization. */
}

void HAL_IncTick(void) {
    uwTick += uwTickFreq;
    librertos_tick_interrupt();
}
