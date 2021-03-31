# How to use LibreRTOS semaphores

## Initializing semaphores

A semaphores must be initialized before it can be used.

In most cases a semaphores (and other structures) are initialized in the main function, before peripherals initialization and before the OS starts scheduling tasks.

```c
struct semaphore_t SemBinary;
struct semaphore_t SemCounting;

void main(void) {
  LibrertosInit();
  timerBegin();
  
  /* Create tasks */

  /* Initialize semaphore */
  SemaphoreInit(&SemBinary, 0, 1);
  SemaphoreInit(&SemCounting, 3, 3);
  
  /* Initialize peripherals */

  LibrertosStart();
  for(;;) {
    LibrertosScheduler();
  }
}
```

## Take semaphore

Tasks or interrupts can take (lock) a semaphore.

### Interrupts

Interrupts that use LibreRTOS API should lock the scheduler in the start and unlock in the end of the interrupt handler.

Not locking the scheduler can make tasks to be scheduled in the middle of the interrupt. Unless your state of mind is "I really know what I am doing!" you should lock the scheduler in interrupts.

```c
void InterruptExample1(void) {
  SchedulerLock();

  /* Take semaphore */
  if(SemaphoreTake(&SemBinary)) {
    /* Success */
  }
  else {
    /* Semaphore was taken */
  }

  SchedulerUnlock();
}
```

### Tasks

In LibreRTOS the tasks must run to completion. That means there is no way to stop the task in the middle to resume it later.

Tasks can take from a semaphore the same way an interrupt would. However tasks can also pend on semaphore. Pending on a semaphore will put the task to sleep if the semaphore is taken and wake it up when the semaphore is given.

```c
void TaskExample1(void* param) {
  /* Take semaphore */
  if(SemaphoreTake(&SemBinary)) {
    /* Success */
  }
  else {
    /* Semaphore was taken */
    
    /* Pend on semaphore, no timeout */
    SemaphorePend(&SemBinary, MAX_DELAY);
  }
}
```

This is a better way to take a semaphore and pend if it is taken (this is equivalent to the code above).

```c
void TaskExample2(void* param) {
  /* Take semaphore, pend if taken */
  if(SemaphoreTakePend(&SemBinary, MAX_DELAY)) {
    /* Success */
  }
  else {
    /* Semaphore taken, task is already pending on semaphore */
  }
}
```

## Give semaphore

Tasks or interrupts can give a semaphore. Pretty similar to taking a semaphore.

### Interrupts

Interrupts that use LibreRTOS API should lock the scheduler in the start and unlock in the end of the interrupt handler.

```c
void InterruptExample2(void) {
  SchedulerLock();

  /* Take semaphore */
  if(SemaphoreGive(&SemBinary)) {
    /* Success */
  }
  else {
    /* Semaphore was taken */
  }

  SchedulerUnlock();
}
```

### Tasks

Tasks can give a semaphore the same way an interrupt would. Tasks cannot pend to give a semaphore, only to take them.

```c
void TaskExample3(void* param) {
  /* Give a semaphore */
  if(SemaphoreGive(&SemBinary)) {
    /* Success */
  }
  else {
    /* Semaphore was given */
  }
}
```
