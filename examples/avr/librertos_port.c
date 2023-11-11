/* Copyright (c) 2016-2023 Djones A. Boni - MIT License */

#include "librertos.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <stdio.h>

#if defined(__AVR_ATmega2560__)
    #define LED_DDR DDRB
    #define LED_PORT PORTB
    #define LED_PIN PINB
    #define LED_BIT (1 << PB7)
#elif defined(__AVR_ATmega328P__)
    #define LED_DDR DDRB
    #define LED_PORT PORTB
    #define LED_PIN PINB
    #define LED_BIT (1 << PB5)

    #define PRR0 PRR
    #define USART0_RX_vect USART_RX_vect
    #define USART0_UDRE_vect USART_UDRE_vect
    #define USART0_TX_vect USART_TX_vect
#endif

/*******************************************************************************
 * LED
 ******************************************************************************/

void led_config(void) {
    LED_DDR |= LED_BIT; /* LED output. */
    led_write(0);
}

void led_write(uint8_t value) {
    if (value) {
        LED_PORT |= LED_BIT; /* LED ON. */
    } else {
        LED_PORT &= ~LED_BIT; /* LED OFF. */
    }
}

void led_toggle(void) {
    LED_PIN = LED_BIT; /* LED toggle. */
}

/*******************************************************************************
 * Serial
 ******************************************************************************/

static int serial_putchar(char var, FILE *stream) {
    serial_write_byte(var);
    return 0;
}

static FILE serial_stdout = FDEV_SETUP_STREAM(serial_putchar, NULL, _FDEV_SETUP_WRITE);

void serial_init(uint32_t speed) {
    uint8_t ucsra = 0;
    uint32_t ubrr;

    ubrr = (F_CPU - 4 * speed) / (8 * speed);

    if (ubrr <= 0x0FFFU) {
        /* Prescaler 8 is OK. */
        ucsra |= (1 << U2X0); /* Prescaler = 8 */
    } else {
        /* Prescaler 8 cannot be used. */
        ubrr = (F_CPU - 8 * speed) / (16 * speed);
        ucsra &= ~(1 << U2X0); /* Prescaler = 16 */
    }

    PRR0 &= ~(1 << PRUSART0); /* Enable UART clock. */

    UCSR0B = 0; /* Disable TX and RX. */

    /* Set speed and other configurations. */
    UBRR0 = ubrr;
    UCSR0A = ucsra;
    UCSR0C = (0 << UPM00) |                /* No parity */
             (0 << USBS0) |                /* 1 Stop bit */
             (3 << UCSZ00);                /* 8 bits data */
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | /* Enable TX and RX. */
             (0 << UCSZ02);                /* 8 bits data */

    stdout = &serial_stdout;
    stderr = &serial_stdout;
}

void serial_write_byte(uint8_t data) {
    while (!(UCSR0A & (1 << UDRE0))) {
    }
    UDR0 = data;
}

int16_t serial_read(void) {
    int16_t data = -1;

    if (UCSR0A & (1 << RXC0)) {
        data = UDR0;
    }

    return data;
}

/* USART0, Rx Complete. */
ISR(USART0_RX_vect) {
}

/* USART0 Data register Empty. */
ISR(USART0_UDRE_vect) {
}

/* USART0, Tx Complete. */
ISR(USART0_TX_vect) {
}

/*******************************************************************************
 * Port and Tick timer
 ******************************************************************************/

void port_init(void) {
    led_config();

    set_sleep_mode(0); /* IDLE */
    sleep_enable();
}

void idle_wait_interrupt(void) {
    __asm __volatile("sleep" ::
                         : "memory");
}

void port_enable_tick_interrupt(void) {
    uint8_t prescaler;

    PRR0 &= ~(1 << PRTIM0); /* Enable timer clock. */

    TIMSK0 = 0; /* Disable timer interrupts. */

    switch (TIMER_PRESCALER) {
    case 1:
        prescaler = 0x01;
        break;
    case 8:
        prescaler = 0x02;
        break;
    case 64:
        prescaler = 0x03;
        break;
    case 256:
        prescaler = 0x04;
        break;
    case 1024:
        prescaler = 0x05;
        break;
    default:
        /* Invalid prescaler value. */
        LIBRERTOS_ASSERT(0, TIMER_PRESCALER, "Invalid timer prescaler.");
        prescaler = 0x05;
    }

    TCCR0A = (0x00 << COM0A0) | /* Normal port operation, OC0A disconnected. */
             (0x00 << COM0B0) | /* Normal port operation, OC0B disconnected. */
             (0x03 << WGM00);   /* Mode: Fast PWM (WGM02:0=0b011). */

    TCCR0B = (0x00 << FOC0A) | (0x00 << FOC0B) | /* */
             (0x00 << WGM02) |                   /* Mode: Fast PWM (WGM02:0=0b011). */
             (prescaler << CS00);                /* Clock source. */

    TCNT0 = 0;             /* Clear counter. */
    TIFR0 = 0xFFU;         /* Clear interrupt flags. */
    TIMSK0 = (1 << TOIE0); /* Enable timer overflow interrupt. */
}

/* Timer/Counter0 Overflow. */
ISR(TIMER0_OVF_vect) {
    task_t *interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
}

/* Timer/Counter0 Compare Match A. */
ISR(TIMER0_COMPA_vect) {
}

/* Timer/Counter0 Compare Match B. */
ISR(TIMER0_COMPB_vect) {
}
