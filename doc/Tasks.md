# Tasks

## Creating Tasks

Tasks can be created at any point during the execution of the program. Therefore
tasks can be created during the initialization and even in other tasks.

Below we show an example of an IDLE task, that just waits for the next interrupt
to happen.

```cpp
/* File: task_idle.c */
void func_task_idle(void *param) {
    idle_wait_interrupt();
}
```

## Delaying Tasks

## Suspending Tasks

## Resuming Tasks
