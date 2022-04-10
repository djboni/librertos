/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos_impl.h"

#include <stddef.h>
#include <string.h>

/*
 * "Unsafe" functions:
 * Some implementation functions read and change data without the protection of
 * a critical section, therefore they do not disable interrupts by themselves.
 * These functions are commented as "Unsafe" and should be used caution.
 */

/*
 * LibreRTOS state.
 */
librertos_t librertos;

/*
 * Initialize LibreRTOS state.
 *
 * Must be called **before** starting the tick timer interrupt.
 */
void librertos_init(void)
{
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        /* Make non-zero, to be easy to spot uninitialized fields. */
        memset(&librertos, 0x5A, sizeof(librertos));

        librertos.tick = 0;
    }
    CRITICAL_EXIT();
}

/*
 * Process a tick timer interrupt.
 *
 * Must be called periodically by the interrupt of a timer.
 */
void tick_interrupt(void)
{
    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        librertos.tick++;
    }
    CRITICAL_EXIT();
}

/*
 * Get the tick count, the number of ticks since initialization.
 *
 * Note that the tick count can overflow.
 */
tick_t get_tick(void)
{
    tick_t tick;

    CRITICAL_VAL();

    CRITICAL_ENTER();
    {
        tick = librertos.tick;
    }
    CRITICAL_EXIT();

    return tick;
}

/* Unsafe. */
void list_init(struct list_t *list)
{
    list->head = (struct node_t *)list;
    list->tail = (struct node_t *)list;
    list->length = 0;
}

/* Unsafe. */
void node_init(struct node_t *node, void *owner)
{
    node->next = NULL;
    node->prev = NULL;
    node->list = NULL;
    node->owner = owner;
}

/* Unsafe. */
void list_insert_after(
    struct list_t *list, struct node_t *pos, struct node_t *node)
{
    /*
     * A  x-- C --x  B
     *   <--------->
     *
     * A = pos
     * C = node
     */

    node->next = pos->next;
    node->prev = pos;

    /*
     * A <--- C ---> B
     *   <--------->
     */

    pos->next->prev = node;

    /*
     * A <--- C <--> B
     *   ---------->
     */

    pos->next = node;

    /*
     * A <--> C <--> B
     */

    node->list = list;
    list->length++;
}

/* Unsafe. */
void list_insert_before(
    struct list_t *list, struct node_t *pos, struct node_t *node)
{
    list_insert_after(list, pos->prev, node);
}

/* Unsafe. */
void list_insert_first(struct list_t *list, struct node_t *node)
{
    list_insert_after(list, LIST_HEAD(list), node);
}

/* Unsafe. */
void list_insert_last(struct list_t *list, struct node_t *node)
{
    list_insert_after(list, list->tail, node);
}

/* Unsafe. */
void list_remove(struct node_t *node)
{
    struct list_t *list = node->list;

    /*
     * A <--> C <--> B
     *
     * A = pos
     * C = node
     */

    node->next->prev = node->prev;

    /*
     * A <--> C ---> B
     *   <----------
     */

    node->prev->next = node->next;

    /*
     * A <--- C ---> B
     *   <--------->
     */

    node->next = NULL;
    node->prev = NULL;

    /*
     * A  x-- C --x  B
     *   <--------->
     */

    node->list = NULL;
    list->length--;
}

/* Unsafe. */
struct node_t *list_get_first(struct list_t *list)
{
    return list->head;
}

/* Unsafe. */
struct node_t *list_get_last(struct list_t *list)
{
    return list->tail;
}
