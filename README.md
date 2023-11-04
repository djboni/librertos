# [LibreRTOS](https://github.com/djboni/librertos)

Copyright (c) 2016-2023 Djones A. Boni - MIT License

LibreRTOS is a portable Single-Stack Real-Time Operating System.

The objective of LibreRTOS is to run a microcontroller application one step
above the bare metal.

Design goals:

- Portability: use of strictly standard C.
  - No use of C compiler extensions, and
  - Assembly is used only to manage interrupts.
- Readability: clean code, easy to read and to reason about.
- Real-Time: all operations that disable interrupts take constant time O(1).
- Single-Stack: all tasks share the same stack, allowing a large number of
  tasks to be created on RAM constrained projects.
  - The tasks must run to completion and must hold state in static memory.
- Static allocation.
  - No malloc/free and such.

Features:

- Provide preemptive and cooperative kernel modes.
- Provide synchronization with queues, semaphores and mutexes.

Notes:

- Interrupts that use LibreRTOS API must lock the scheduler at the beginning and
  unlock it before returning.

## Using This Project

0. Clone this project using `git` or download the project as a
   [zip file](https://github.com/djboni/librertos/archive/refs/heads/master.zip)
   and extract it
1. Add `scr/librertos.c` to the build
2. Add `include/` to the include directories
3. Add `#include librertos.h` to the source files

## Running Tests

This project contains unit tests.

1. Install the dependencies (Ubuntu 20.04 and 22.04):

```sh
sudo apt install git make gcc g++ dh-autoreconf gcovr
```

2. Clone the project with `git` and initialize the submodules:

```sh
git clone https://github.com/djboni/librertos
cd librertos
git submodule update --init
```

One command to build and to run the unit-tests:

```sh
make run_tests
```

The end of the test log should be something like this (no failures):

```
...
==============================================
10 Tests, 0 Inexistent, 0 Failures
OK
```

The test framework CppUTest is build automatically when you first run the tests.
However, the build can fail if is any of the necessary programs or libraries is
missing.

It is also possible to run the tests and check the code coverage information:

```sh
make run_coverage
```

If you want to run a specific test use the script `misc/run_tests.sh` and pass
the **test** path. Example:

```sh
misc/run_tests.sh tests/src/semaphore_test.cpp
```
