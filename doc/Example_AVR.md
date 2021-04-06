# Example AVR

Microcontroller ATmega328P.

```c
#include "librertos.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU            16000000UL
#define TIMER_PRESCALER  64
#define TICKS_PER_SECOND ((tick_t)(F_CPU / (256.0 * TIMER_PRESCALER)))

/* Timer0 configuration.
Code borrowed from Arduinutil.
https://github.com/djboni/arduinutil
 */
void timerBegin(void) {
    uint8_t prescaler;

    PRR &= ~(1U << PRTIM0); /* Enable timer clock. */

    TIMSK0 = 0U; /* Disable timer interrupts. */

    switch(TIMER_PRESCALER) {
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
        prescaler = 0x05U;
        ASSERT(0); /* Invalid prescaler value */
    }

    TCCR0A =
            (0x00U << COM0A0) | /* Normal port operation, OC0A disconnected. */
            (0x00U << COM0B0) | /* Normal port operation, OC0B disconnected. */
            (0x03U << WGM00); /* Mode: Fast PWM (WGM02:0=0b011). */

    TCCR0B =
            (0x00U << FOC0A) |
            (0x00U << FOC0B) |
            (0x00U << WGM02) | /* Mode: Fast PWM (WGM02:0=0b011). */
            (prescaler << CS00); /* Clock source. */

    TCNT0 = 0U; /* Clear counter. */
    TIFR0 = 0xFFU; /* Clear interrupt flags. */
    TIMSK0 = (1U << TOIE0); /* Enable timer overflow interrupt. */
}

/* Timer0 overflow ISR. */
ISR(TIMER0_OVF_vect) {
    LibrertosTick();
}

struct task_t TcbTaskA;

/* TaskA code. Toggle PORTB5 (Arduino LED). */
void FuncTaskA(void* param) {
    (void)param;

    /* Toggle PORTB5 (Arduino LED). */
    PORTB ^= (1 << PORTB5);

    /* Delay for half a second. */
    TaskDelay(0.5 * TICKS_PER_SECOND);
}

int main(void) {
    /* Initialize LibreRTOS data structures. */
    LibrertosInit();

    /* Start Timer0. */
    timerBegin();

    /* PORTB5 Output (Arduino LED). */
    DDRB |= (1 << DDB5);

    /* Create TaskA. TaskControlBlock=TcbTaskA, Function=FuncTaskA, Priority=0, Parameter=0. */
    LibrertosTaskCreate(&TcbTaskA, 0, &FuncTaskA, NULL);

    /* Start scheduler. */
    LibrertosStart();

    /* Run scheduler. */
    for(;;) {
        LibrertosScheduler();
    }
}
```
