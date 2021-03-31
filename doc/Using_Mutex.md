# How to use LibreRTOS mutex

Mutex can only be locked by tasks.

The implementation allows a mutex to be unlocked by interrupts, but I do not see any use for that.

## Initializing mutex

A mutex must be initialized before it can be used.

In most cases a mutex (and other structures) are initialized in the main function, before peripherals initialization and before the OS starts scheduling tasks.

```c
struct mutex_t Mtx;

void main(void) {
  LibrertosInit();
  timerBegin();
  
  /* Create tasks */

  /* Initialize queue */
  MutexInit(&Mtx);
  
  /* Initialize peripherals */

  LibrertosStart();
  for(;;) {
    LibrertosScheduler();
  }
}
```

## Task locking and unlocking a mutex

In LibreRTOS the tasks must run to completion. That means there is no way to stop the task in the middle to resume it later.

Tasks can lock and pend on a mutex. Pending-read on a mutex will put the task to sleep if the mutex is locked and wake it up when the mutex is unlocked.

```c
void TaskExample1(void* param) {
    /* Try to lock mutex */
    if(MutexLock(&Mtx)) {
        /* Mutex locked */

        /* Do stuff with mutual exclusion... */

        /* Unlock mutex */
        MutexUnlock(&Mtx);
    }
    else {
        /* Mutex was locked */
    }
}
```

```c
void TaskExample2(void* param) {
    /* Try to lock mutex */
    if(MutexLockPend(&Mtx, MAX_DELAY)) {
        /* Mutex locked */

        /* Do stuff with mutual exclusion... */

        /* Unlock mutex */
        MutexUnlock(&Mtx);
    }
    else {
        /* Mutex was locked, task is already pending on mutex */
    }
}
```

## Peripheral shared between two tasks

Sometimes two or more tasks share the same peripheral and must use a mutex to protect the peripheral from concurrent access.

```c
void FunctionCalledByTask1AndTask2(void) {
  uint8_t buff[QUEISZ];
  
  /* Lock peripheral's mutex, pend on it if already locked */
  if(MutexLockPend(&Mtx, MAX_DELAY)) {
    /* Mutex locked */

    /* Read from task's queue, pend on it if empty */
    if(QueueReadPend(&que, buff, MAX_DELAY)) {
      /* Write data to peripheral */
      WriteToPeripheral(buff);
    }
  
    /* Unlock mutex */
    MutexUnlock(&Mtx);
  }
}
```
