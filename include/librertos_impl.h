/* Copyright (c) 2022 Djones A. Boni - MIT License */

#ifndef LIBRERTOS_IMPL_H_
#define LIBRERTOS_IMPL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "librertos.h"

typedef struct
{
    tick_t tick;
    task_t *current_task;
    struct list_t tasks_ready[NUM_PRIORITIES];
    struct list_t tasks_suspended;
} librertos_t;

extern librertos_t librertos;

#define LIST_HEAD(list) ((struct node_t *)list)

void list_init(struct list_t *list);
void node_init(struct node_t *node, void *owner);
void list_insert_first(struct list_t *list, struct node_t *node);
void list_insert_last(struct list_t *list, struct node_t *node);
void list_insert_after(
    struct list_t *list, struct node_t *pos, struct node_t *node);
void list_insert_before(
    struct list_t *list, struct node_t *pos, struct node_t *node);
void list_remove(struct node_t *node);
struct node_t *list_get_first(struct list_t *list);
struct node_t *list_get_last(struct list_t *list);
uint8_t list_empty(struct list_t *list);
void list_move_first_to_last(struct list_t *list);

#ifdef __cplusplus
}
#endif

#endif /* LIBRERTOS_IMPL_H_ */
