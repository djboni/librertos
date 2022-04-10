# [LibreRTOS](https://github.com/djboni/librertos)

Copyright (c) 2022 Djones A. Boni - MIT License

LibreRTOS is a portable Single-Stack Real-Time Operating System.

The objective of LibreRTOS is to run a microcontroller application one step
above the bare metal.

Design goals:

- Portability: use of strictly standard C.
- Readability: clean code, easy to read and reason about.
- Real-Time: all operations that disable interrupts take constant time O(1).
- Single-Stack: all tasks share the same stack, allowing a larger number of
  tasks to be created on RAM constrained projects.
- Static allocation.

Features:

- Provide preemptive and cooperative kernel modes.
- Provide synchronization with queues, semaphores and mutexes.

## Cloning This Project

Clone and initialize the submodules:

```sh
$ git clone https://github.com/djboni/librertos.git --recurse-submodules
```

If you cloned without `--recurse-submodules`, you can manually initialize the
submodules:

```sh
$ git submodule update --init
```

## Running Tests

It is simple to build and run the unit-tests:

```sh
$ make run_tests    # Run tests
$ make run_coverage # Run tests and print coverage
```

If you want to run a specific test use the script `misc/run_tests.sh` and pass
the **source** path:

```sh
$ misc/run_tests.sh src/semaphore.c
```

## Things to Be Aware of

- Due to using the a single-stack, tasks must run to completion and also must
  hold its state into static memory.
- Interrupts that use LibreRTOS API must lock the scheduler at the beginning and
  unlock it before returning.
- Strict standard C:
  - No C compiler extensions and
  - Assembly is used on the port layer to manage interrupts.
