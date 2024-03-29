/* Copyright (c) 2016-2023 Djones A. Boni - MIT License */

#include "librertos.h"
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

static pthread_t tick_interrupt;
static sem_t idle_wakeup;
static pthread_mutex_t mutual_exclusion;

static void *func_tick_interrupt(void *param) {
    int retval;
    INTERRUPTS_VAL();
    (void)param;

    while (1) {
        task_t *interrupted_task;

        usleep(1000000 * TICK_PERIOD);

        INTERRUPTS_DISABLE();
        interrupted_task = interrupt_lock();
        librertos_tick_interrupt();
        interrupt_unlock(interrupted_task);
        INTERRUPTS_ENABLE();

        retval = sem_post(&idle_wakeup);
        LIBRERTOS_ASSERT(retval == 0, "func_tick_interrupt(): Could not unlock (post) semaphore.");
    }

    return NULL;
}

void port_init(void) {
    int retval;
    pthread_mutexattr_t attr;

    retval = sem_init(&idle_wakeup, 0, 0);
    LIBRERTOS_ASSERT(retval == 0, "port_init(): Could not initialize semaphore.");

    retval = pthread_mutexattr_init(&attr);
    retval |= pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    retval |= pthread_mutex_init(&mutual_exclusion, &attr);
    retval |= pthread_mutexattr_destroy(&attr);
    LIBRERTOS_ASSERT(retval == 0, "port_init(): Could not initialize mutex.");
}

void port_enable_tick_interrupt(void) {
    int retval =
        pthread_create(&tick_interrupt, NULL, &func_tick_interrupt, NULL);
    LIBRERTOS_ASSERT(retval == 0, "port_enable_tick_interrupt(): Could not create tick thread.");
}

void idle_wait_interrupt(void) {
    int retval = sem_wait(&idle_wakeup);
    LIBRERTOS_ASSERT(retval == 0, "idle_wait_interrupt(): Could not lock (wait) semaphore.");
}

void INTERRUPTS_DISABLE(void) {
    int retval = pthread_mutex_lock(&mutual_exclusion);
    LIBRERTOS_ASSERT(retval == 0, "INTERRUPTS_DISABLE(): Could not lock mutex.");
}

void INTERRUPTS_ENABLE(void) {
    int retval = pthread_mutex_unlock(&mutual_exclusion);
    LIBRERTOS_ASSERT(retval == 0, "INTERRUPTS_ENABLE(): Could not unlock mutex.");
}

void CRITICAL_ENTER(void) {
    int retval = pthread_mutex_lock(&mutual_exclusion);
    LIBRERTOS_ASSERT(retval == 0, "CRITICAL_ENTER(): Could not lock mutex.");
}

void CRITICAL_EXIT(void) {
    int retval = pthread_mutex_unlock(&mutual_exclusion);
    LIBRERTOS_ASSERT(retval == 0, "CRITICAL_EXIT(): Could not unlock mutex.");
}
