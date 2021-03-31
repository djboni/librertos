# [LibreRTOS](https://github.com/djboni/librertos)

by [Djones A. Boni](https://professoreletrico.com)

LibreRTOS is a portable single-stack Real Time Operating System.

Provides preemptive, cooperative and hybrid kernel modes. In hybrid mode only
tasks with priority above a value can cause a preemption.

All tasks share the same stack. This allows a large number of tasks to be
created even on RAM constrained projects.

Tasks must run to completion and also must hold its state into static varibales.

Interrupts that use LibreRTOS API must lock the scheduler at the beginning and
unlock before returning.

# Features

* Single-stack
* Preemptive, cooperative or hybrid kernel
* Semaphore
* Queue (message queue)
* Fifo (character queue)
* Mutex (recursive, no priority inheritance mechanism)
* Software timers (one-shot, periodic or no-period)
* Documented source files
* Unit-tested:
  * [LibreRTOS Test with Python (New)](https://github.com/djboni/librertos/tree/master/test) 
  * [LibreRTOS Test with Boost.Test (Old)](https://github.com/djboni/librertos_test)

## How to use LibreRTOS

LibreRTOS uses only standard C90 and can be used in any architecture with a few macro definitions with assembly code.

User must provide a few things:

1) A `projdefs.h` file, which defines LibreRTOS configurations
and basic macros for interrupts and critical section management.
For examples look at `port/projdefs_*.h`

2) A periodic tick interrupt.

Below there is a simple initialization template.
For a complete and working example take a look in our
[AVR example](https://github.com/djboni/librertos/blob/master/doc/Example_AVR.md).

```c
#include "librertos.h"

/* Task structures */
struct task_t Task_ADC;
struct task_t Task_UART;
struct task_t Task_IDLE;

/* Task functions */
extern void FuncTaskADC(void *param);
extern void FuncTaskUART(void *param);
extern void FuncTaskIDLE(void *param);

int main(void)
{
    /* Initialize LibreRTOS data structures. */
    LibrertosInit();

    /* Start tick timer and initialize peripherals. */

    /* Create tasks:
    Structure, priority, function, parameter.
    */
    LibrertosTaskCreate(&Task_ADC, 2, &FuncTaskADC, NULL);
    LibrertosTaskCreate(&Task_UART, 1, &FuncTaskUART, NULL);
    LibrertosTaskCreate(&Task_IDLE, 0, &FuncTaskIDLE, NULL);

    /* Start and run scheduler. */
    LibrertosStart();
    for(;;) {
        LibrertosScheduler();
    }
}
```

# Operation mode

* Tasks must run to completion
    * For cooperative kernel: no other task (even higher priority) will run until the task returns
* Local variables are lost between calls
* Interrupts that use LibreRTOS API must lock the scheduler at the beginning and unlock before returning

## Tasks must run to completion

```c
/* DON'T use infinite loops.

TaskDelay() and other "pending" function do not stop the task function. They
make the task not to be scheduled again until the specified conditions are met
(such as time). The function must return.
*/
void FuncTaskADC(void *param) {
    for(;;) {
        int16_t adc_val = ReadADC();

        if (adc_val >= 0) {
            /* ADC available.
            Process value...
            */
        }

        TaskDelay(0.5 * TICKS_PER_SECOND);
    }
}

/* DO run to completion (make the task function return). */
void FuncTaskADC(void *param) {
    int16_t adc_val = ReadADC();

    if (adc_val >= 0) {
        /* ADC available.
            Process value...
        */
    }

    TaskDelay(0.5 * TICKS_PER_SECOND);
}
```

## Local variables are lost between calls

```c
/* DON'T use local variables if they need to keep their value.

Local variables are in the stack and, when another task runs, they are
overwritten with other values.

This function always initializes last_adc_val to zero. If you use
"int16_t last_adc_val;" instead then the value will be garbage from other tasks.
 */
void FuncTaskADC(void *param) {
    int16_t last_adc_val = 0;
    int16_t adc_val = ReadADC();

    if (adc_val >= 0) {
        if(adc_val - last_adc_val >= 0) {
            /* ADC value went up. */
        }

        last_adc_val = adc_val;
    }

    TaskDelay(0.5 * TICKS_PER_SECOND);
}

/* DO use static variables to keep their values.

Static variables are stored together with global variables, but are accessible
only in the function.

This function always initializes last_adc_val to zero. If you use
"int16_t last_adc_val;" instead then the value will be garbage from other tasks.
*/
void FuncTaskADC(void *param) {
    static int16_t last_adc_val;
    int16_t adc_val = ReadADC();

    if (adc_val >= 0) {
        if(adc_val - last_adc_val >= 0) {
            /* ADC value went up. */
        }

        last_adc_val = adc_val;
    }

    TaskDelay(0.5 * TICKS_PER_SECOND);
}
```

## Interrupts that use LibreRTOS API must lock the scheduler at the beginning and unlock before returning

```c
struct semaphore_t Sem_UART0;
struct fifo_t Fifo_UART0;
static uint8_t Fifo_Buff_UART0[32];

void StructuresUART0Init(void) {
    SemaphoreInit(&Sem_UART0, 0, 1);
    FifoInit(&Fifo_UART0, &Fifo_Buff_UART0, sizeof(Fifo_Buff_UART0));
}

/* DON'T forget to lock and unlock the scheduler.

A task may resume an run in FifoWrite() call, and the interrupt processing is not finished yet.
*/
void UART0Interrupt(void *param) {
    uint8_t data = GetUART0Data();
    FifoWrite(&Fifo_UART0, &data, 1);
    /* A task may run now and the interrupt is not fully processed. */
    SemaphoreGive(&Sem_UART0);
}

/* DO lock and unlock the scheduler.

Tasks will resume only on scheduler unlock call.
*/
void UART0Interrupt(void *param) {
    SchedulerLock();

    uint8_t data = GetUART0Data();
    FifoWrite(&Fifo_UART0, &data, 1);

    SemaphoreGive(&Sem_UART0);

    SchedulerUnlock();
    /* Only now tasks can run. */
}
```

# Contributing to LibreRTOS

If you have suggestions for improving LibreRTOS, please
[open an issue or pull request on GitHub](https://github.com/djboni/librertos).

# Licensing

You can use LibreRTOS both for closed- and open-source projects. You are also
free to keep changes to yourself.

You are free to copy, modify, and distribute LibreRTOS with attribution under
the terms of the
[Apache License Version 2.0](http://www.apache.org/licenses/LICENSE-2.0).
See the doc/LICENSE file for details.

# Important files

[README.md](https://github.com/djboni/librertos/blob/master/README.md)
This file.

[doc/LICENCE](https://github.com/djboni/librertos/blob/master/doc/LICENSE)
Complete license text.

[doc](https://github.com/djboni/librertos/tree/master/doc)
Documentation and examples.
