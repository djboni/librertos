# Memory Usage

- Microcontroller: ARM STM32F103C8
- Compiler: GCC
- Tick: 32 bits

## RAM Usage

| Element   | Data Size          |
| --------- | ------------------ |
| LibreRTOS | 56 + 8\*priorities |
| Task      | 48                 |
| Semaphore | 16                 |
| Mutex     | 16                 |
| Queue     | 24 + buff          |

- Size in bytes.

## Flash Usage

| Kernel Mode | Optimization | Tasks and Events | Only Tasks |
| ----------- | ------------ | ---------------- | ---------- |
| Preemptive  | -Os (size)   | 5332             | 4128       |
| Cooperative | -Os (size)   | 5128             | 4016       |
| Preemptive  | -O1          | 5888             | 4628       |
| Cooperative | -O1          | 5708             | 4524       |

- Size in bytes.
- The column "Tasks and Events" includes code for tasks, semaphores, mutexes,
  and queues.
- The column "Only Tasks" includes only the code for tasks.
