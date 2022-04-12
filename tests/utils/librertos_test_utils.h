/* Copyright (c) 2022 Djones A. Boni - MIT License */

#ifndef LIBRERTOS_TEST_UTILS_H_
#define LIBRERTOS_TEST_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "librertos.h"
#include "librertos_impl.h"

#include <cstring>

void time_travel(tick_t ticks_to_the_future);
void set_tick(tick_t tick);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

    #define BUFF_SIZE 128

struct Param
{
    char *buff;
    const char *start, *resume, *end;
    task_t *task_to_resume;
    int suspend_after_n_runs;

    static void task_sequencing(void *param)
    {
        Param *p = (Param *)param;

        if (p == NULL)
            task_suspend(CURRENT_TASK_PTR);

        // Concat start
        strcat(p->buff, p->start);

        // Resume task
        if (p->task_to_resume != NULL)
            task_resume(p->task_to_resume);

        // Concat resume
        strcat(p->buff, p->resume);

        // Suspend itself
        if (--(p->suspend_after_n_runs) <= 0)
            task_suspend(CURRENT_TASK_PTR);

        // Concat end
        strcat(p->buff, p->end);
    }
};

struct ParamSemaphore
{
    char *buff;
    semaphore_t *sem;
    const char *start, *locked, *unlocked, *end;

    static void task_sequencing(void *param)
    {
        ParamSemaphore *p = (ParamSemaphore *)param;

        if (p == NULL)
            task_suspend(CURRENT_TASK_PTR);

        // Concat start
        strcat(p->buff, p->start);

        if (semaphore_lock(p->sem))
        {
            // Concat unlocked
            strcat(p->buff, p->unlocked);
            task_suspend(CURRENT_TASK_PTR);
        }
        else
        {
            // Concat locked
            strcat(p->buff, p->locked);
            semaphore_suspend(p->sem);
        }

        // Concat end
        strcat(p->buff, p->end);
    }
};

struct ParamMutex
{
    char *buff;
    mutex_t *mtx;
    const char *start, *locked, *unlocked, *end;

    static void task_sequencing(void *param)
    {
        ParamMutex *p = (ParamMutex *)param;

        if (p == NULL)
            task_suspend(CURRENT_TASK_PTR);

        // Concat start
        strcat(p->buff, p->start);

        if (mutex_lock(p->mtx))
        {
            // Concat unlocked
            strcat(p->buff, p->unlocked);
            task_suspend(CURRENT_TASK_PTR);
        }
        else
        {
            // Concat locked
            strcat(p->buff, p->locked);
            mutex_suspend(p->mtx);
        }

        // Concat end
        strcat(p->buff, p->end);
    }
};

struct ParamQueue
{
    char *buff;
    queue_t *que;
    const char *start, *empty, *not_empty, *end;

    static void task_sequencing(void *param)
    {
        ParamQueue *p = (ParamQueue *)param;
        int8_t data;

        if (p == NULL)
            task_suspend(CURRENT_TASK_PTR);

        // Concat start
        strcat(p->buff, p->start);

        if (queue_read(p->que, &data))
        {
            // Concat not empty
            strcat(p->buff, p->not_empty);
            task_suspend(CURRENT_TASK_PTR);
        }
        else
        {
            // Concat empty
            strcat(p->buff, p->empty);
            queue_suspend_read(p->que);
        }

        // Concat end
        strcat(p->buff, p->end);
    }
};

#endif /* __cplusplus */

#endif /* LIBRERTOS_TEST_UTILS_H_ */
