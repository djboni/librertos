# LibreRTOS on AVR

- Board: Arduino Mega 2560
- Microcontroller: AVR ATmega2560
- Compiler: GCC
- Tick: 16 bits

1. Install the dependencies (Ubuntu 22.04):

```sh
sudo apt install make gcc-avr avr-libc avrdude
```

2. Compile:

```sh
make main.hex
```

3. Write flash:

```sh
make flash
```

## RAM Usage

| Element   | Data Size          |
| --------- | ------------------ |
| LibreRTOS | 25 + 5\*priorities |
| Task      | 25                 |
| Semaphore | 7                  |
| Mutex     | 8                  |
| Queue     | 17 + buff          |

- Size in bytes.

## Flash Usage

| Kernel Mode | Optimization | Tasks and Events | Only Tasks |
| ----------- | ------------ | ---------------- | ---------- |
| Preemptive  | -Os (size)   | 3720             | 1948       |
| Cooperative | -Os (size)   | 3456             | 1872       |
| Preemptive  | -O1          | 3856             | 2100       |
| Cooperative | -O1          | 3626             | 2014       |

- Size in bytes.
- The column "Tasks and Events" includes code for tasks, semaphores, mutexes,
  and queues.
- The column "Only Tasks" includes only the code for tasks.
