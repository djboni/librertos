/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos.h"

#include <string.h>

void queue_init(queue_t *que, void *buff, uint8_t que_size, uint8_t item_size)
{
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        que->free = que_size;
        que->used = 0;
        que->head = 0;
        que->tail = 0;
        que->item_size = item_size;
        que->end = que_size * item_size;
        que->buff = (uint8_t *)buff;
    }
    CRITICAL_EXIT();
}

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

result_t queue_write(queue_t *que, const void *data)
{
    result_t result = FAIL;
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
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
    }
    CRITICAL_EXIT();

    return result;
}

uint8_t queue_numfree(queue_t *que)
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

uint8_t queue_numused(queue_t *que)
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

uint8_t queue_isempty(queue_t *que)
{
    return queue_numused(que) == 0;
}

uint8_t queue_isfull(queue_t *que)
{
    return queue_numfree(que) == 0;
}

uint8_t queue_numitems(queue_t *que)
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

uint8_t queue_itemsize(queue_t *que)
{
    /* Queue item size should not change.
     * Critical section not necessary. */
    return que->item_size;
}
