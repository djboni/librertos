# How to use LibreRTOS queues

## Initializing queue

A queue must be initialized before it can be used.

It needs a queue structure pointer, a data buffer, the number of items the queue can hold and the size of the items in bytes.

In most cases a queue (and other structures) are initialized in the main function, before peripherals initialization and before the OS starts scheduling tasks.

```c
#define QUELEN  4 /* Number of items in the queue */
#define QUEISZ 16 /* Size of each item */

struct queue_t Que; /* Queue structure */
static uint8_t Que_Buffer[QUELEN * QUEISZ]; /* Queue buffer */

void main(void) {
  LibrertosInit();
  timerBegin();
  
  /* Create tasks */

  /* Initialize queue */
  
  /* Initialize peripherals */

  LibrertosStart();
  for(;;) {
    LibrertosScheduler();
  }
}
```

## Read data from queue

Tasks or interrupts can read data from a queue.

### Interrupts

Interrupts that use LibreRTOS API should lock the scheduler in the start and unlock in the end of the interrupt handler.

Not locking the scheduler can make tasks to be scheduled in the middle of the interrupt. Unless your state of mind is "I really know what I am doing!" you should lock the scheduler in interrupts.

```c
void InterruptExample1(void) {
  uint8_t buff[QUEISZ];

  SchedulerLock();
  
  /* Read data from queue into buff */
  if(QueueRead(&Que, buff)) {
    /* Process buff */
  }
  else {
    /* Queue was empty */
  }
  
  SchedulerUnlock();
}
```

### Tasks

In LibreRTOS the tasks must run to completion. That means there is no way to stop the task in the middle to resume it later.

Tasks can read from a queue the same way an interrupt would. However tasks can also pend-read on queues. Pending-read on a queue will put the task to sleep if the queue is empty and wake it up when the queue is not empty anymore.

```c
void TaskExample1(void* param) {
  uint8_t buff[QUEISZ];

  /* Read data from queue into buff */
  if(QueueRead(&Que, buff)) {
    /* Process buff */
  }
  else {
    /* Queue was empty */
    
    /* Pend-read on queue, no timeout */
    QueuePendRead(&Que, MAX_DELAY);
  }
}
```

This is a better way to read from a queue and pend-read if it is empty (this is equivalent to the code above).

```c
void TaskExample2(void* param) {
  uint8_t buff[QUEISZ];

  /* Read data from queue into buff, pend-read if empty */
  if(QueueReadPend(&Que, buff, MAX_DELAY)) {
    /* Process buff */
  }
  else {
    /* Queue was empty, task is already pending-read on queue */
  }
}
```

## Write data to queue

Tasks or interrupts can write data to a queue. Pretty similar to reading from queue.

### Interrupts

Interrupts that use LibreRTOS API should lock the scheduler in the start and unlock in the end of the interrupt handler.

```c
void InterruptExample2(void) {
  uint8_t buff[QUEISZ];

  SchedulerLock();
  
  /* Put something into buff */
  
  /* Write data from buff to queue */
  if(QueueWrite(&Que, buff)) {
    /* Data written to queue */
  }
  else {
    /* Queue was full */
  }
  
  SchedulerUnlock();
}
```

### Tasks

Tasks can write to a queue the same way an interrupt would. However tasks can also pend-write on queues. Pending-write on a queue will put the task to sleep if the queue is full and wake it up when the queue is not full anymore.

```c
void TaskExample3(void* param) {
  uint8_t buff[QUEISZ];
  
  /* Put something into buff */

  /* Write data from buff to queue */
  if(QueueWrite(&Que, buff)) {
    /* Data written to queue */
  }
  else  {
    /* Queue was full */
    
    /* Pend-write on queue */
    Queue_pendWrite(&Que, MAX_DELAY);
  }
}
```

This is a better way to write to a queue and pend-write if it is full (this is equivalent to the code above).

```c
void TaskExample4(void* param) {
  uint8_t buff[QUEISZ];
  
  /* Put something into buff */

  /* Write data from buff to queue, pend-write if full */
  if(QueueWritePend(&Que, buff, MAX_DELAY)) {
    /* Data written to queue */
  }
  else {
    /* Queue was full, task is already pending-write on queue */
  }
}
```
