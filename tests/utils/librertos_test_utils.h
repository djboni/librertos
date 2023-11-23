/* Copyright (c) 2016-2023 Djones A. Boni - MIT License */

#ifndef LIBRERTOS_TEST_UTILS_H_
#define LIBRERTOS_TEST_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define LIBRERTOS_DEBUG_DECLARATIONS
#include "librertos.h"

#include <string.h>

void time_travel(tick_t ticks_to_the_future);
void set_tick(tick_t tick);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

    #define BUFF_SIZE 128

struct Param {
    char *buff;
    const char *start, *resume, *end;
    task_t *task_to_resume;
    int suspend_after_n_runs;

    static void task_sequencing(void *param) {
        Param *p = (Param *)param;

        if (p == NULL) {
            task_suspend(NULL);
            return;
        }

        // Concat start
        strcat(p->buff, p->start);

        // Resume task
        if (p->task_to_resume != NULL)
            task_resume(p->task_to_resume);

        // Concat resume
        strcat(p->buff, p->resume);

        // Suspend itself
        if (--(p->suspend_after_n_runs) <= 0)
            task_suspend(NULL);

        // Concat end
        strcat(p->buff, p->end);
    }

    static void task_sequencing_lock_scheduler(void *param) {
        scheduler_lock();
        task_sequencing(param);
        scheduler_unlock();
    }
};

struct ParamSemaphore {
    char *buff;
    semaphore_t *sem;
    const char *start, *locked, *unlocked, *end;

    static void task_sequencing(void *param) {
        ParamSemaphore *p = (ParamSemaphore *)param;

        if (p == NULL) {
            task_suspend(NULL);
            return;
        }

        // Concat start
        strcat(p->buff, p->start);

        if (semaphore_lock(p->sem)) {
            // Concat unlocked
            strcat(p->buff, p->unlocked);
            task_suspend(NULL);
        } else {
            // Concat locked
            strcat(p->buff, p->locked);
            semaphore_suspend(p->sem, MAX_DELAY);
        }

        // Concat end
        strcat(p->buff, p->end);
    }

    static void task_semaphore_lock_suspend(void *param) {
        ParamSemaphore *p = (ParamSemaphore *)param;

        if (p == NULL) {
            task_suspend(NULL);
            return;
        }

        // Concat start
        strcat(p->buff, p->start);

        if (semaphore_lock_suspend(p->sem, MAX_DELAY)) {
            // Concat unlocked
            strcat(p->buff, p->unlocked);
        } else {
            // Concat locked
            strcat(p->buff, p->locked);
        }

        // Concat end
        strcat(p->buff, p->end);
    }
};

struct ParamMutex {
    char *buff;
    mutex_t *mtx;
    const char *start, *locked, *unlocked, *end;

    static void task_sequencing(void *param) {
        ParamMutex *p = (ParamMutex *)param;

        if (p == NULL) {
            task_suspend(NULL);
            return;
        }

        // Concat start
        strcat(p->buff, p->start);

        if (mutex_lock(p->mtx)) {
            // Concat unlocked
            strcat(p->buff, p->unlocked);
            task_suspend(NULL);

            // If the task runs again it can lock again the recursive mutex.
            // So we just unlock mutex and suspend the task.
            mutex_unlock(p->mtx);
            task_suspend(NULL);
        } else {
            // Concat locked
            strcat(p->buff, p->locked);
            mutex_suspend(p->mtx, MAX_DELAY);
        }

        // Concat end
        strcat(p->buff, p->end);
    }

    static void task_mutex_lock_suspend(void *param) {
        ParamMutex *p = (ParamMutex *)param;

        if (p == NULL) {
            task_suspend(NULL);
            return;
        }

        // Concat start
        strcat(p->buff, p->start);

        if (mutex_lock_suspend(p->mtx, MAX_DELAY)) {
            // Concat unlocked
            strcat(p->buff, p->unlocked);

            // If the task runs again it can lock again the recursive mutex.
            // So we just unlock mutex and suspend the task.
            mutex_unlock(p->mtx);
            task_suspend(NULL);
        } else {
            // Concat locked
            strcat(p->buff, p->locked);
        }

        // Concat end
        strcat(p->buff, p->end);
    }
};

struct ParamQueue {
    char *buff;
    queue_t *que;
    const char *start, *empty, *not_empty, *end;

    static void task_sequencing(void *param) {
        ParamQueue *p = (ParamQueue *)param;
        int8_t data;

        if (p == NULL) {
            task_suspend(NULL);
            return;
        }

        // Concat start
        strcat(p->buff, p->start);

        if (queue_read(p->que, &data)) {
            // Concat not empty
            strcat(p->buff, p->not_empty);
            task_suspend(NULL);
        } else {
            // Concat empty
            strcat(p->buff, p->empty);
            queue_suspend(p->que, MAX_DELAY);
        }

        // Concat end
        strcat(p->buff, p->end);
    }

    static void task_queue_read_suspend(void *param) {
        ParamQueue *p = (ParamQueue *)param;
        int8_t data;

        if (p == NULL) {
            task_suspend(NULL);
            return;
        }

        // Concat start
        strcat(p->buff, p->start);

        if (queue_read_suspend(p->que, &data, MAX_DELAY)) {
            // Concat not empty
            strcat(p->buff, p->not_empty);
        } else {
            // Concat empty
            strcat(p->buff, p->empty);
        }

        // Concat end
        strcat(p->buff, p->end);
    }
};

#endif /* __cplusplus */

#endif /* LIBRERTOS_TEST_UTILS_H_ */
