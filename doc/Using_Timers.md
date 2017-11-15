# How to use LibreRTOS timers

## Types of timer

The timer type defines how it will behave when it runs.

- TIMERTYPE_ONESHOT One-shot timers, after reset, will run once after 'period' ticks have passed.
- TIMERTYPE_AUTO Auto-reset timers (or periodic timers), after reset, will run once every 'period' ticks have passed.
- TIMERTYPE_NOPERIOD No-period timers are faster one-shot timers with
period zero. They run once as soon as the timer task is scheduled.

One-shot timers are used to execute code if the timer is not reset in time, as a timeout callback. As an example: one can use a one-shot timer to shutdown the processor after some time no buttons are pressed, reseting the timer every button press.

Auto-reset timers are used for periodic tasks, such as blinking a LED, read push-buttons, watchdog reset/feed.

No-period timers offer an alternative for processing in interrupt context. Instead, the interrupt starts a no-period timer, which will process the data in task context.

## Initializing timer

A timer must be initialized before it can be used.

It needs a timer structure pointer, a type, a period, a callback function and a callback parameter.

After initialization a timer is inactive (stopped), therefore it will not run unless it is reset or started.

In most cases a timer (and other structures) are initialized in the main function, before peripherals initialization and before the OS starts scheduling tasks.

```c
struct Timer_t timerBlink; /* Timer structure */

void func_timerBlink(struct Timer_t* timer, void* param)
{
  LED_TOGGLE(); /* digitalWrite(LED, !digitalRead(LED)) */
}

void main(void)
{
  OS_init();
  timerBegin();
  
  /* Create timer task */
  OS_timerTaskCreate(LIBRERTOS_MAX_PRIORITY - 1);
  
  /* Create other tasks */

  /* Initialize timer */
  Timer_init(&timerBlink, TIMERTYPE_AUTO, MS_TO_TICKS(500), &func_timerBlink,
      (void*)0);
  
  /* Start timer */
  Timer_start(&timerBlink);
  
  /* Initialize peripherals */
  LED_CONFIG(); /* pinMode(LED, OUTPUT) */

  OS_start();
  for(;;)
    OS_scheduler();
}
```

## Timer reset

Reseting a timer will make it active. After reset the next run will be after 'period' ticks have passed.

One-shot timers will be stopped when they run.

Auto-reset timers (periodic) will be reset when they run.

No-period timers will be stopped when they run.

```c
Timer_reset(&timer);
```

## Timer start

Starting a timer will reset it only if it is inactive.

```c
Timer_start(&timer);
```

## Timer stop

Stopping a timer will make it inactive. It will not run anymore, unless it is reset or started.

```c
Timer_stop(&timer);
```
