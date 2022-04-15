/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos.h"
#include "librertos_impl.h"

#include <string.h>

/*
 * Initialize queue.
 *
 * Parameters:
 *   - buff: pointer to buffer with size (que_size * item_size).
 *   - que_size: number of items the queue can hold.
 *   - item_size: size of the items the queue manages.
 */
void queue_init(queue_t *que, void *buff, uint8_t que_size, uint8_t item_size)
{
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        /* Make non-zero, to be easy to spot uninitialized fields. */
        memset(que, 0x5A, sizeof(*que));

        que->free = que_size;
        que->used = 0;
        que->head = 0;
        que->tail = 0;
        que->item_size = item_size;
        que->end = que_size * item_size;
        que->buff = (uint8_t *)buff;
        event_init(&que->event_write);
    }
    CRITICAL_EXIT();
}

/*
 * Read an item from queue.
 *
 * Parameters:
 *   - data: pointer to buffer where to write.
 *
 * Returns: 1 if success, 0 otherwise.
 */
result_t queue_read(queue_t *que, void *data)
{
    result_t result = FAIL;
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        if (que->used > 0)
        {
            uint8_t *buff = &que->buff[que->tail];

            memcpy(data, buff, que->item_size);

            que->tail += que->item_size;
            if (que->tail >= que->end)
                que->tail = 0;

            que->free++;
            que->used--;
            result = SUCCESS;
        }
    }
    CRITICAL_EXIT();

    return result;
}

/*
 * Write an item to queue.
 *
 * Parameters:
 *   - data: pointer to buffer where to read.
 *
 * Returns: 1 if success, 0 otherwise.
 */
result_t queue_write(queue_t *que, const void *data)
{
    result_t result = FAIL;
    CRITICAL_VAL();

    CRITICAL_ENTER();

    if (que->free > 0)
    {
        uint8_t *buff = &que->buff[que->head];

        memcpy(buff, data, que->item_size);

        que->head += que->item_size;
        if (que->head >= que->end)
            que->head = 0;

        que->free--;
        que->used++;
        result = SUCCESS;
    }

    scheduler_lock();
    event_resume_task(&que->event_write);
    CRITICAL_EXIT();
    scheduler_unlock();

    return result;
}

/*
 * Get number of free items in the queue.
 */
uint8_t queue_get_num_free(queue_t *que)
{
    uint8_t value;
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        value = que->free;
    }
    CRITICAL_EXIT();

    return value;
}

/*
 * Get number of used items in the queue.
 */
uint8_t queue_get_num_used(queue_t *que)
{
    uint8_t value;
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        value = que->used;
    }
    CRITICAL_EXIT();

    return value;
}

/*
 * Check if the queue is empty (zero items used).
 */
uint8_t queue_is_empty(queue_t *que)
{
    return queue_get_num_used(que) == 0;
}

/*
 * Check if the queue is full (all items used).
 */
uint8_t queue_is_full(queue_t *que)
{
    return queue_get_num_free(que) == 0;
}

/*
 * Get total number of items in the queue (free + used).
 */
uint8_t queue_get_num_items(queue_t *que)
{
    uint8_t value;
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        value = que->free + que->used;
    }
    CRITICAL_EXIT();

    return value;
}

/*
 * Get the size of the items in the queue.
 */
uint8_t queue_get_item_size(queue_t *que)
{
    /* Queue item size should not change.
     * Critical section not necessary. */
    return que->item_size;
}

/*
 * Suspend task on queue waiting to read.
 */
void queue_suspend(queue_t *que)
{
    CRITICAL_VAL();

    CRITICAL_ENTER();

    if (que->used == 0)
    {
        scheduler_lock();
        event_suspend_task(&que->event_write);
        CRITICAL_EXIT();
        scheduler_unlock();
    }
    else
    {
        CRITICAL_EXIT();
    }
}

/*
 * Read an item from queue if not empty, else suspend the task on the queue.
 *
 * Parameters:
 *   - data: pointer to buffer where to write.
 *
 * Returns: 1 if successfully read the queue, 0 otherwise (suspended).
 */
result_t queue_read_suspend(queue_t *que, void *data)
{
    result_t result;

    result = queue_read(que, data);
    if (result == FAIL)
        queue_suspend(que);

    return result;
}
