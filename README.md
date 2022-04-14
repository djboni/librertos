# [LibreRTOS](https://github.com/djboni/librertos)

Copyright (c) 2022 Djones A. Boni - MIT License

LibreRTOS is a portable Single-Stack Real-Time Operating System.

The objective of LibreRTOS is to run a microcontroller application one step
above the bare metal.

Design goals:

- Portability: use of strictly standard C.
- Readability: clean code, easy to read and to reason about.
- Real-Time: all operations that disable interrupts take constant time O(1).
- Single-Stack: all tasks share the same stack, allowing a large number of
  tasks to be created on RAM constrained projects.
- Static allocation.

Features:

- Provide preemptive and cooperative kernel modes.
- Provide synchronization with queues, semaphores and mutexes.

## Cloning This Project

Clone and initialize the submodules:

```sh
git clone https://github.com/djboni/librertos.git --recurse-submodules
```

If you cloned without `--recurse-submodules`, you can initialize the submodules
manually:

```sh
git submodule update --init
```

## Running Tests

It is simple to build and to run the unit-tests:

```sh
make run_tests    # Run tests
make run_coverage # Run tests and print coverage
```

The end of the test log should be something like this (no failures):

```
...
==============================================
9 Tests, 0 Inexistent, 0 Failures
OK
```

If you want to run a specific test use the script `misc/run_tests.sh` and pass
the **test** path. Example:

```sh
misc/run_tests.sh tests/src/semaphore_test.cpp
```

The test framework CppUTest is build automatically when you first run the tests.
However, the build can fail if is any of the necessary programs or libraries is
missing.

To install dependencies to run the tests on Ubuntu 20.04:

```sh
sudo apt update
sudo apt install git make gcc g++ dh-autoreconf gcovr
```

## Things to Be Aware of

- Due to be using a single-stack, the tasks must run to completion and they also
  must hold their state in static memory.
- Interrupts that use LibreRTOS API must lock the scheduler at the beginning and
  unlock it before returning.
- Strict standard C:
  - No use of C compiler extensions and
  - Assembly is used only in the port layer to manage interrupts.
