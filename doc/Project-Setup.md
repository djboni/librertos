# Project Setup

This document describes how to setup your project to use LibreRTOS:

- [Port File - `librertos_port.h`](Project-Setup#port-file)
  How to port LibreRTOS to new hardware
- [Project File - `librertos_proj.h`](Project-Setup#project-file)
  How to configure LibreRTOS in your project
- [Initialization and Scheduler Loop - `main()`](Project-Setup#initialization-and-scheduler-loop)
  How to start LibreRTOS in your project
- [Tick Interrupt](Project-Setup#tick-interrupt)
  How to call the tick interrupt for timekeeping

## Port File

The objective of the port file `librertos_port.h` is to port LibreRTOS to the
hardware in use. In other words, it is the way to show LibreRTOS how to deal
with the hardware you are using.

The port file must define the following macros:

- Forcing interrupt state:
  - `#define INTERRUPTS_VAL() ...` - Local variable for the interrupt state,
    generally not used (can be useful when debugging)
  - `#define INTERRUPTS_DISABLE() ...` - Force interrupts to be disabled
  - `#define INTERRUPTS_ENABLE() ...` - Force interrupts to be enabled
- Nested interrupt management:
  - `#define CRITICAL_VAL() ...` - Local variable for the interrupt state
  - `#define CRITICAL_ENTER() ...` - Save the interrupt state and disable interrupts
  - `#define CRITICAL_EXIT() ...` - Restore the saved interrupt state

These macros contain all the assembly language that LibreRTOS needs to work.

A practical example for ARM is shown below. More examples, including AVR and
Linux can be found in the [examples/](../examples/) directory.

```cpp
/* File: arm/librertos_port.h */
#define INTERRUPTS_VAL() /* Empty, not used */
#define INTERRUPTS_DISABLE() __disable_irq()
#define INTERRUPTS_ENABLE() __enable_irq()

#define CRITICAL_VAL() uint32_t __istate_val
#define CRITICAL_ENTER() \
    __istate_val = __get_PRIMASK(); \
    __disable_irq()
#define CRITICAL_EXIT() __set_PRIMASK(__istate_val)
```

Other macros and functions can also be defined in the port file, such as:

- `port_init()` - Initializes the interrupts, tick timer, serial/stdout, etc
- `port_enable_tick_interrupt()` - Enables the tick timer interrupt
- `idle_wait_interrupt()` - IDLE the CPU while waiting for an interrupt

## Project File

The objective of the project file `librertos_proj.h` is configure LibreRTOS
for your project, such as the type of kernel, number of tasks and tick interrupt.
In other words, it is the way to customize LibreRTOS for your project needs.

The project file must define the following macros and types:

- `#define LIBRERTOS_ASSERT(expr, val, msg) ...` - Macro used for assertions
  (ex: `assert(expr)`)
- `#define KERNEL_MODE ...` - Kernel mode selection, can be defined as:
  - `LIBRERTOS_PREEMPTIVE` - Preemptive kernel mode: the current highest priority
    task will run and a **higher priority task** will be scheduled
    **as soon as it is ready to run**
  - `LIBRERTOS_COOPERATIVE` - Cooperative kernel mode: the current highest priority
    task will run but a **higher priority task** will be scheduled
    **only when the current task finishes**
- `#define NUM_PRIORITIES ...` - Number of priorities for the tasks (integer > 0).
  For convenience the values below are defined:
  - `LOW_PRIORITY = 0`
  - `HIGH_PRIORITY = NUM_PRIORITIES - 1`
- `typedef ... tick_t;` - Unsigned type for the tick counter
- `typedef ... difftick_t;` - Signed type to calculate tick counter differences

Other macros and functions can also be defined in the project file, such as:

- `#define TICKS_PER_SECOND ...`
- `#define TICK_PERIOD (1.0 / TICKS_PER_SECOND)`

A practical example for ARM is shown below. More examples, including AVR and
Linux can be found in the [examples/](../examples/) directory.

```cpp
/* File: arm/librertos_proj.h */
#define LIBRERTOS_ASSERT(expr, val, msg) /* Empty */

#define KERNEL_MODE LIBRERTOS_PREEMPTIVE
#define NUM_PRIORITIES 8

typedef uint32_t tick_t;
typedef int32_t difftick_t;

#define TICKS_PER_SECOND 1000
#define TICK_PERIOD (1.0 / TICKS_PER_SECOND)
```

## Initialization and Scheduler Loop

To initialize LibreRTOS we need to call `librertos_init()`.
From this point we are able to create tasks.
More information on tasks can be found in the [Tasks Documentation](Tasks).

Before running the scheduler, we need to initialize the hardware and enable
the tick timer interrupt.

Then we call `librertos_start()` to enable the scheduler and finally
we call the scheduler `librertos_sched()` in a loop.

The code below shows an example of initialization.

```cpp
/* File: main.c */
#include "librertos.h"
#include <stddef.h>

/* Allocate task structures. */
task_t task_idle;
task_t task_blink;

/* Declara the task functions. */
extern void func_task_idle(void *param);
extern void func_task_blink(void *param);

int main(void) {
    /* Initialize LibreRTOS. */
    librertos_init();

    /* Create tasks (passing a null pointer as task parameter). */
    librertos_create_task(LOW_PRIORITY, &task_idle, &func_task_idle, NULL);
    librertos_create_task(HIGH_PRIORITY, &task_blink, &func_task_blink, NULL);

    /* Initialize the hardware. */
    port_init();

    /* Start tick timer interrupt. */
    port_enable_tick_interrupt();

    /* Enable LibreRTOS then call the scheduler in a loop. */
    librertos_start();
    while (1) {
        librertos_sched();
    }

    return 0;
}
```

## Tick Interrupt

The tick timer interrupt is used by LibreRTOS for timekeeping.
For that reason it is necessary a periodic interrupt that calls the function
`librertos_tick_interrupt()`.

LibreRTOS does not know about clock time ("seconds" or "minutes"), it knows
about "ticks". Each tick which is equivalent the interrupt period of the
timer. The values of `TICKS_PER_SECOND` and `TICK_PERIOD` are defined to
convert between timer ticks and clock time.

The code examples below show how to process a tick in an interrupt on
Arduino ATmega2560 (AVR) and ARM Cortex-M3.

```cpp
/* File: avr/librertos_port.c */
#include "librertos.h"

ISR(TIMER0_OVF_vect) {
    task_t *interrupted_task = interrupt_lock();
    librertos_tick_interrupt();
    interrupt_unlock(interrupted_task);
}
```

```cpp
/* File: arm/librertos_port.c */
#include "librertos.h"

void SysTick_Handler(void) {
    task_t *interrupted_task = interrupt_lock();
    librertos_tick_interrupt();

    /* Interrupt processing finished.
     * When using ARM, after processing the interrupt, we need to clear it
     * before doing non-interrupt work, such as calling interrupt_unlock(),
     * which can call the scheduler and execute tasks.
     */
    NVIC_ClearPendingIRQ(SysTick_IRQn);

    interrupt_unlock(interrupted_task);
}
```
