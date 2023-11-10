# [LibreRTOS](https://github.com/djboni/librertos)

Copyright (c) 2016-2023 Djones A. Boni - MIT License

LibreRTOS is a portable Single-Stack Real-Time Operating System.

The main objective of LibreRTOS is to run in microcontrollers.

Features:

- Preemptive and cooperative kernel modes.
- Tasks and timers with priorities.
- Synchronization with queues, semaphores and mutexes.

Design goals:

- Portability:
  - Strictly standard C (no compiler extensions), and
  - Minimal assembly required (only 4 macros to manage interrupts).
- Real-Time: all operations that disable interrupts take constant time O(1).
- Single-Stack: all tasks share the same stack, allowing a large number of
  tasks to be created on RAM constrained projects.
  - The tasks must run to completion and must hold state in static memory.
- Static allocation: no use of malloc/free and such.

Notes:

- Interrupts that use LibreRTOS API must lock the scheduler at the beginning and
  unlock it before returning.

## Using LibreRTOS in Your Project

1. Get the code:
   - Clone this project using Git, or
   - Download LibreRTOS as a
     [zip file](https://github.com/djboni/librertos/archive/refs/heads/master.zip)
     and extract it.
2. Copy `src/librertos.c` and `include/librertos.h` to your project
3. Create a port file `librertos_port.h` for your hardware (see the [examples](examples/))
4. Create a project file `librertos_proj.h` for your project (see the [examples](examples/))
5. Add `#include "librertos.h"` to your source files and use LibreRTOS
6. Initialize and use LibreRTOS in your project (see more in the [documentation](doc/home))

## Examples and Documentation

- The user documentation is available in the [doc/](doc/home) directory.
- Examples can be found in the [examples/](examples/) directory
- Implementation details are documented in the code.
