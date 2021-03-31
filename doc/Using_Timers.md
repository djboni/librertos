# How to use LibreRTOS timers

## Types of timer

The timer type defines how it will behave when it runs.

- TIMERTYPE_ONESHOT When reset, one-shot timers will run once after 'period' ticks have passed.
- TIMERTYPE_AUTO When reset, auto-reset timers (or periodic timers) will run once every 'period' ticks have passed.
- TIMERTYPE_NOPERIOD No-period timers are faster one-shot timers with
period zero. They run once as soon as the timer task is scheduled.

One-shot timers are used to execute code if the timer is not reset in time, as a timeout callback. As an example: one can use a one-shot timer to shutdown the processor after some time no buttons are pressed, reseting the timer every button press.

Auto-reset timers are used for periodic tasks, such as blinking a LED, read push-buttons, watchdog reset/feed.

No-period timers offer an alternative for processing in interrupt context. Instead, the interrupt starts a no-period timer, which will process the data in task context instead of interrupt context.

## Initializing timer

A timer must be initialized before it can be used.

It needs a timer structure pointer, a type, a period, a callback function and a callback parameter.

After initialization a timer is inactive (stopped), therefore it will not run unless it is reset or started.

In most cases a timer (and other structures) are initialized in the main function, before peripherals initialization and before the OS starts scheduling tasks.

```c
struct Timer_t Timer_Blink; /* Timer structure */

void FuncTimerBlink(struct Timer_t* timer, void* param) {
  LED_TOGGLE(); /* digitalWrite(LED, !digitalRead(LED)) */
}

void main(void) {
  LibrertosInit();
  Timer0Begin();
  
  /* Create timer task */
  LibrertosTimerTaskCreate(LIBRERTOS_MAX_PRIORITY - 1);
  
  /* Create other tasks */

  /* Initialize timer */
  TimerInit(&Timer_Blink, TIMERTYPE_AUTO, MS_TO_TICKS(500), &FuncTimerBlink,
      NULL);
  
  /* Start timer */
  TimerStart(&Timer_Blink);
  
  /* Initialize peripherals */
  LED_CONFIG(); /* pinMode(LED, OUTPUT) */

  LibrertosStart();
  for(;;) {
    LibrertosScheduler();
  }
}
```

## Timer reset

Reseting a timer will make it active. After reset the next run will be after 'period' ticks have passed.

One-shot timers will be stopped when they run.

Auto-reset timers (periodic) will be reset when they run.

No-period timers will be stopped when they run.

```c
TimerReset(&timer);
```

## Timer start

Starting a timer will reset it only if it is inactive.

```c
TimerStart(&timer);
```

## Timer stop

Stopping a timer will make it inactive. It will not run anymore, unless it is reset or started.

```c
TimerStop(&timer);
```
