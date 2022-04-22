/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos.h"
#include "librertos_impl.h"

#include <string.h>

/*
 * Initialize the queue.
 *
 * @param buff Pointer to a buffer with size que_size*item_size.
 * @param que_size Number of items the queue can hold.
 * @param item_size Size of the items in the queue.
 */
void queue_init(queue_t *que, void *buff, uint8_t que_size, uint8_t item_size)
{
    CRITICAL_VAL();

    CRITICAL_ENTER();

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

    CRITICAL_EXIT();
}

/* Call with interrupts disabled. */
static uint8_t queue_can_be_read(queue_t *que)
{
    return que->used > 0;
}

/* Call with interrupts disabled. */
static uint8_t queue_can_be_written(queue_t *que)
{
    return que->free > 0;
}

/*
 * Read an item from the queue.
 *
 * @param data Pointer to a buffer with size que->item_size.
 * @return 1 with success, 0 otherwise.
 */
result_t queue_read(queue_t *que, void *data)
{
    result_t result = FAIL;
    CRITICAL_VAL();

    CRITICAL_ENTER();

    if (queue_can_be_read(que))
    {
        memcpy(data, &que->buff[que->tail], que->item_size);

        que->tail += que->item_size;
        if (que->tail >= que->end)
            que->tail = 0;

        que->free++;
        que->used--;
        result = SUCCESS;
    }

    CRITICAL_EXIT();

    return result;
}

/*
 * Write an item to the queue.
 *
 * @param data Pointer to a buffer with size que->item_size.
 * @return 1 with success, 0 otherwise.
 */
result_t queue_write(queue_t *que, const void *data)
{
    result_t result = FAIL;
    CRITICAL_VAL();

    CRITICAL_ENTER();

    if (queue_can_be_written(que))
    {
        scheduler_lock();

        memcpy(&que->buff[que->head], data, que->item_size);

        que->head += que->item_size;
        if (que->head >= que->end)
            que->head = 0;

        que->free--;
        que->used++;
        result = SUCCESS;

        event_resume_task(&que->event_write);

        CRITICAL_EXIT();
        scheduler_unlock();
    }
    else
    {
        CRITICAL_EXIT();
    }

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

    value = que->free;

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

    value = que->used;

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

    value = que->free + que->used;

    CRITICAL_EXIT();

    return value;
}

/*
 * Get the size of the items in the queue.
 */
uint8_t queue_get_item_size(queue_t *que)
{
    /* Queue item size should not change.
     * Critical section is not necessary. */
    return que->item_size;
}

/*
 * Suspend the task waiting to read the queue, waiting a maximum number for
 * ticks to pass before resuming.
 *
 * This function can be used only by tasks.
 *
 * @param ticks_to_delay Number of ticks to delay resuming the task. Pass
 * MAX_DELAY to wait forever.
 */
void queue_suspend(queue_t *que, tick_t ticks_to_delay)
{
    CRITICAL_VAL();

    CRITICAL_ENTER();

    if (!queue_can_be_read(que))
    {
        scheduler_lock();

        event_delay_task(&que->event_write, ticks_to_delay);

        CRITICAL_EXIT();
        scheduler_unlock();
    }
    else
    {
        CRITICAL_EXIT();
    }
}

/*
 * Read an item from the the queue if not empty, else tries to suspend the task
 * waiting to read the queue, waiting a maximum number for ticks to pass before
 * resuming.
 *
 * This function can be used only by tasks.
 *
 * @param data Pointer to a buffer with size que->item_size.
 * @param ticks_to_delay Number of ticks to delay resuming the task. Pass
 * MAX_DELAY to wait forever.
 * @return 1 with success reading, 0 otherwise.
 */
result_t queue_read_suspend(queue_t *que, void *data, tick_t ticks_to_delay)
{
    result_t result;

    result = queue_read(que, data);
    if (result == FAIL)
        queue_suspend(que, ticks_to_delay);

    return result;
}
