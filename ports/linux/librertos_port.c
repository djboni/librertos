/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos.h"
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static pthread_t main_thread;
static pthread_t tick_interrupt;
static sem_t idle_wakeup;
static sem_t interrupt_go;
static pthread_mutex_t mutual_exclusion;

static void sig_handler(int sig_num)
{
    int retval;
    INTERRUPTS_VAL();

    if (sig_num == SIGUSR1)
    {
        retval = sem_post(&interrupt_go);
        LIBRERTOS_ASSERT(
            retval == 0,
            retval,
            "func_tick_interrupt(): Could not unlock (post) semaphore.");

        INTERRUPTS_DISABLE();
        librertos_tick_interrupt();
        INTERRUPTS_ENABLE();

        retval = sem_post(&idle_wakeup);
        LIBRERTOS_ASSERT(
            retval == 0,
            retval,
            "func_tick_interrupt(): Could not unlock (post) semaphore.");
    }
    else
    {
        printf("sig_handler(): Caught the wrong signal number [%d].", sig_num);
        exit(sig_num);
    }
}

static void *func_tick_interrupt(void *param)
{
    int retval;
    INTERRUPTS_VAL();
    (void)param;

    for (;;)
    {
        usleep(1000000 * TICK_PERIOD);

        INTERRUPTS_DISABLE();

        pthread_kill(main_thread, SIGUSR1);

        retval = sem_wait(&interrupt_go);
        LIBRERTOS_ASSERT(
            retval == 0,
            retval,
            "func_tick_interrupt(): Could not lock (wait) semaphore.");

        INTERRUPTS_ENABLE();
    }

    return NULL;
}

void port_init(void)
{
    int retval;
    pthread_mutexattr_t attr;

    retval = sem_init(&idle_wakeup, 0, 0);
    LIBRERTOS_ASSERT(
        retval == 0, retval, "port_init(): Could not initialize semaphore.");

    retval = sem_init(&interrupt_go, 0, 0);
    LIBRERTOS_ASSERT(
        retval == 0, retval, "port_init(): Could not initialize semaphore.");

    main_thread = pthread_self();
    signal(SIGUSR1, sig_handler);

    retval = pthread_mutexattr_init(&attr);
    retval |= pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    retval |= pthread_mutex_init(&mutual_exclusion, &attr);
    retval |= pthread_mutexattr_destroy(&attr);
    LIBRERTOS_ASSERT(
        retval == 0, retval, "port_init(): Could not initialize mutex.");
}

void port_enable_tick_interrupt(void)
{
    int retval =
        pthread_create(&tick_interrupt, NULL, &func_tick_interrupt, NULL);
    LIBRERTOS_ASSERT(
        retval == 0,
        retval,
        "port_enable_tick_interrupt(): Could not create tick thread.");
}

void idle_wait_interrupt(void)
{
    int retval = sem_wait(&idle_wakeup);
    LIBRERTOS_ASSERT(
        retval == 0,
        retval,
        "idle_wait_interrupt(): Could not lock (wait) semaphore.");
}

void INTERRUPTS_DISABLE(void)
{
    int retval = pthread_mutex_lock(&mutual_exclusion);
    LIBRERTOS_ASSERT(
        retval == 0, retval, "INTERRUPTS_DISABLE(): Could not lock mutex.");
}

void INTERRUPTS_ENABLE(void)
{
    int retval = pthread_mutex_unlock(&mutual_exclusion);
    LIBRERTOS_ASSERT(
        retval == 0, retval, "INTERRUPTS_ENABLE(): Could not unlock mutex.");
}

void CRITICAL_ENTER(void)
{
    int retval = pthread_mutex_lock(&mutual_exclusion);
    LIBRERTOS_ASSERT(
        retval == 0, retval, "CRITICAL_ENTER(): Could not lock mutex.");
}

void CRITICAL_EXIT(void)
{
    int retval = pthread_mutex_unlock(&mutual_exclusion);
    LIBRERTOS_ASSERT(
        retval == 0, retval, "CRITICAL_EXIT(): Could not unlock mutex.");
}
