# [LibreRTOS](https://github.com/djboni/librertos)

by [Djones A. Boni](https://twitter.com/djonesboni)


LibreRTOS is a portable single-stack Real Time Operating System.

All tasks share the same stack, allowing a large number or tasks
to be created even on architectures with low RAM, such as ATmega328P (2kB).

The tasks must follow a "run to completion" scheme, usually using a state
machine.

Upon creation each task is given a different priority.
The priority is used by the scheduler to choose which task will run.
The higher priority task that is ready to run will be scheduled.

LibreRTOS may be configured as preemptive or cooperative mode.
In cooperative mode the current task running must finish before another task
runs (schedule only after completion).
In preemptive mode the current task running may be stopped to run a higher
priority task, resuming the low priority task as soon as the higher priority
task finishes (schedule higher priority task as soon as possible).
In both cases the scheduler chooses the highest priority ready task to run.



LibreRTOS provides:

* Task delay
* Events
* Pend on events
* Pend with timeout

Events available:

* Semaphore
* Queue (variable length data)
* Fifo (character queue)
* Mutex (no priority inheritance mechanism)



You can use LibreRTOS both for closed- and open-source projects. You are also
free to keep changes to this library to yourself. However we'll enjoy your
improvements and suggestions. `:-)`

You are free to copy, modify, and distribute LibreRTOS with attribution under
the terms of the
[Apache License Version 2.0](http://www.apache.org/licenses/LICENSE-2.0).
See the doc/LICENSE file for details.


## How to use LibreRTOS

LibreRTOS uses only standard C and can be used in any architecture.

User must provide two things:

1) A `projdefs.h` file, which defines LibreRTOS configurations
and basic macros for interrupts and critical section management.
For examples look at `port/projdefs_*.h`

2) A periodic tick interrupt.

Below there is a simple initialization template.
For a complete and working example take a look in our
[AVR example](https://github.com/djboni/librertos/blob/master/doc/Example_AVR.md).

```c
#include "LibreRTOS.h"

void Timer_Interrupt(void)
{
    OS_tick();
}

int main(void)
{
    /* Initialize LibreRTOS data structures. */
    OS_init();

    /* Start timer */
    /* Initialize peripherals */
    /* Create tasks */

    /* Start scheduler. */
    OS_start();

    /* Run scheduler. */
    for(;;)
    {
        OS_scheduler();
    }
}
```


## Contributing to LibreRTOS

If you have suggestions for improving LibreRTOS, please
[open an issue or pull request on GitHub](https://github.com/djboni/librertos).


## Important files

[README.md](https://github.com/djboni/librertos/blob/master/README.md)
Fast introduction (this file).

[doc/LICENCE](https://github.com/djboni/librertos/blob/master/doc/LICENSE)
Complete license text.

[doc](https://github.com/djboni/librertos/tree/master/doc)
Documentation and examples.
