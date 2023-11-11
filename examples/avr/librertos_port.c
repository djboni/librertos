/* Copyright (c) 2016-2023 Djones A. Boni - MIT License */

#include "librertos.h"
#include <avr/interrupt.h>
#include <avr/io.h>

#ifdef __AVR_ATmega2560__
    #define PRR PRR0

    #define LED_DDR DDRB
    #define LED_PORT PORTB
    #define LED_PIN PINB
    #define LED_BIT (1 << PB7)
#endif

void led_config(void) {
    LED_DDR |= LED_BIT; /* LED output. */
    led_off();
}

void led_on(void) {
    LED_PORT |= LED_BIT; /* LED ON. */
}

void led_off(void) {
    LED_PORT &= ~LED_BIT; /* LED OFF. */
}

void led_toggle(void) {
    LED_PIN = LED_BIT; /* LED toggle. */
}

void port_init(void) {
    led_config();
}

void idle_wait_interrupt(void) {
}

void port_enable_tick_interrupt(void) {
    uint8_t prescaler;

    PRR &= ~(1U << PRTIM0); /* Enable timer clock. */

    TIMSK0 = 0U; /* Disable timer interrupts. */

    switch (TIMER_PRESCALER) {
    case 1U:
        prescaler = 0x01U;
        break;
    case 8U:
        prescaler = 0x02U;
        break;
    case 64U:
        prescaler = 0x03U;
        break;
    case 256U:
        prescaler = 0x04U;
        break;
    case 1024U:
        prescaler = 0x05U;
        break;
    default:
        /* Invalid prescaler value. */
        LIBRERTOS_ASSERT(
            0,
            TIMER_PRESCALER,
            "port_enable_tick_interrupt(): Invalid timer prescaler.");
        prescaler = 0x05U;
    }

    TCCR0A = (0x00U << COM0A0) | /* Normal port operation, OC0A disconnected. */
             (0x00U << COM0B0) | /* Normal port operation, OC0B disconnected. */
             (0x03U << WGM00);   /* Mode: Fast PWM (WGM02:0=0b011). */

    TCCR0B = (0x00U << FOC0A) | (0x00U << FOC0B) | (0x00U << WGM02) | /* Mode: Fast PWM (WGM02:0=0b011). */
             (prescaler << CS00);                                     /* Clock source. */

    TCNT0 = 0U;             /* Clear counter. */
    TIFR0 = 0xFFU;          /* Clear interrupt flags. */
    TIMSK0 = (1U << TOIE0); /* Enable timer overflow interrupt. */
}

/* Timer0 overflow ISR. */
ISR(TIMER0_OVF_vect) {
    task_t *interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
}
