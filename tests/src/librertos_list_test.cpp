/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos.h"
#include "librertos_impl.h"

/*
 * Main file: src/librertos.c
 * Also compile: tests/mocks/librertos_assert.cpp
 * Also compile: tests/utils/librertos_test_utils.cpp
 */

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

TEST_GROUP (List)
{
    list_t list;
    node_t node1, node2, node3;
    int owner1, owner2, owner3;

    void setup()
    {
        list_init(&list);
        node_init(&node1, &owner1);
        node_init(&node2, &owner2);
        node_init(&node3, &owner3);
    }
    void teardown() {}
};

TEST(List, Initialization)
{
    // List head
    POINTERS_EQUAL(&list, list.head);
    POINTERS_EQUAL(&list, list.tail);
    LONGS_EQUAL(0, list.length);

    // Node 1
    POINTERS_EQUAL(NULL, node1.next);
    POINTERS_EQUAL(NULL, node1.prev);
    POINTERS_EQUAL(NULL, node1.list);
    POINTERS_EQUAL(&owner1, node1.owner);

    // Node 2
    POINTERS_EQUAL(NULL, node2.next);
    POINTERS_EQUAL(NULL, node2.prev);
    POINTERS_EQUAL(NULL, node2.list);
    POINTERS_EQUAL(&owner2, node2.owner);

    // Node 3
    POINTERS_EQUAL(NULL, node3.next);
    POINTERS_EQUAL(NULL, node3.prev);
    POINTERS_EQUAL(NULL, node3.list);
    POINTERS_EQUAL(&owner3, node3.owner);
}

TEST(List, InsertFirst_1_First)
{
    list_insert_first(&list, &node1);

    // List head
    POINTERS_EQUAL(&node1, list.head);
    POINTERS_EQUAL(&node1, list.tail);
    LONGS_EQUAL(1, list.length);

    // Node 1
    POINTERS_EQUAL(&list, node1.next);
    POINTERS_EQUAL(&list, node1.prev);
    POINTERS_EQUAL(&list, node1.list);
}

TEST(List, InsertLast_1_Last)
{
    list_insert_last(&list, &node1);

    // List head
    POINTERS_EQUAL(&node1, list.head);
    POINTERS_EQUAL(&node1, list.tail);
    LONGS_EQUAL(1, list.length);

    // Node 1
    POINTERS_EQUAL(&list, node1.next);
    POINTERS_EQUAL(&list, node1.prev);
    POINTERS_EQUAL(&list, node1.list);
}

TEST(List, InsertFirst_2_First)
{
    list_insert_first(&list, &node1);
    list_insert_first(&list, &node2);

    // List head
    POINTERS_EQUAL(&node2, list.head);
    POINTERS_EQUAL(&node1, list.tail);
    LONGS_EQUAL(2, list.length);

    // Node 1
    POINTERS_EQUAL(&list, node1.next);
    POINTERS_EQUAL(&node2, node1.prev);
    POINTERS_EQUAL(&list, node1.list);

    // Node 2
    POINTERS_EQUAL(&node1, node2.next);
    POINTERS_EQUAL(&list, node2.prev);
    POINTERS_EQUAL(&list, node2.list);
}

TEST(List, InsertLast_2_Last)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);

    // List head
    POINTERS_EQUAL(&node1, list.head);
    POINTERS_EQUAL(&node2, list.tail);
    LONGS_EQUAL(2, list.length);

    // Node 1
    POINTERS_EQUAL(&node2, node1.next);
    POINTERS_EQUAL(&list, node1.prev);
    POINTERS_EQUAL(&list, node1.list);

    // Node 2
    POINTERS_EQUAL(&list, node2.next);
    POINTERS_EQUAL(&node1, node2.prev);
    POINTERS_EQUAL(&list, node2.list);
}

TEST(List, InsertFirst_3_First)
{
    list_insert_first(&list, &node1);
    list_insert_first(&list, &node2);
    list_insert_first(&list, &node3);

    // List head
    POINTERS_EQUAL(&node3, list.head);
    POINTERS_EQUAL(&node1, list.tail);
    LONGS_EQUAL(3, list.length);

    // Node 1
    POINTERS_EQUAL(&list, node1.next);
    POINTERS_EQUAL(&node2, node1.prev);
    POINTERS_EQUAL(&list, node1.list);

    // Node 2
    POINTERS_EQUAL(&node1, node2.next);
    POINTERS_EQUAL(&node3, node2.prev);
    POINTERS_EQUAL(&list, node2.list);

    // Node 3
    POINTERS_EQUAL(&node2, node3.next);
    POINTERS_EQUAL(&list, node3.prev);
    POINTERS_EQUAL(&list, node3.list);
}

TEST(List, InsertLast_3_Last)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);
    list_insert_last(&list, &node3);

    // List head
    POINTERS_EQUAL(&node1, list.head);
    POINTERS_EQUAL(&node3, list.tail);
    LONGS_EQUAL(3, list.length);

    // Node 1
    POINTERS_EQUAL(&node2, node1.next);
    POINTERS_EQUAL(&list, node1.prev);
    POINTERS_EQUAL(&list, node1.list);

    // Node 2
    POINTERS_EQUAL(&node3, node2.next);
    POINTERS_EQUAL(&node1, node2.prev);
    POINTERS_EQUAL(&list, node2.list);

    // Node 3
    POINTERS_EQUAL(&list, node3.next);
    POINTERS_EQUAL(&node2, node3.prev);
    POINTERS_EQUAL(&list, node3.list);
}

TEST(List, InsertAfter_3_AfterPos)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);
    list_insert_after(&list, &node1, &node3);

    // List head
    POINTERS_EQUAL(&node1, list.head);
    POINTERS_EQUAL(&node2, list.tail);
    LONGS_EQUAL(3, list.length);

    // Node 1
    POINTERS_EQUAL(&node3, node1.next);
    POINTERS_EQUAL(&list, node1.prev);
    POINTERS_EQUAL(&list, node1.list);

    // Node 2
    POINTERS_EQUAL(&list, node2.next);
    POINTERS_EQUAL(&node3, node2.prev);
    POINTERS_EQUAL(&list, node2.list);

    // Node 3
    POINTERS_EQUAL(&node2, node3.next);
    POINTERS_EQUAL(&node1, node3.prev);
    POINTERS_EQUAL(&list, node3.list);
}

TEST(List, InsertBefore_3_BeforePos)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);
    list_insert_before(&list, &node2, &node3);

    // List head
    POINTERS_EQUAL(&node1, list.head);
    POINTERS_EQUAL(&node2, list.tail);
    LONGS_EQUAL(3, list.length);

    // Node 1
    POINTERS_EQUAL(&node3, node1.next);
    POINTERS_EQUAL(&list, node1.prev);
    POINTERS_EQUAL(&list, node1.list);

    // Node 2
    POINTERS_EQUAL(&list, node2.next);
    POINTERS_EQUAL(&node3, node2.prev);
    POINTERS_EQUAL(&list, node2.list);

    // Node 3
    POINTERS_EQUAL(&node2, node3.next);
    POINTERS_EQUAL(&node1, node3.prev);
    POINTERS_EQUAL(&list, node3.list);
}

TEST(List, GetFirstNode)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);
    list_insert_last(&list, &node3);

    POINTERS_EQUAL(&node1, list_get_first(&list));
}

TEST(List, GetLastNode)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);
    list_insert_last(&list, &node3);

    POINTERS_EQUAL(&node3, list_get_last(&list));
}

TEST(List, Remove_1_First)
{
    list_insert_last(&list, &node1);

    list_remove(list_get_first(&list));

    // List head
    POINTERS_EQUAL(&list, list.head);
    POINTERS_EQUAL(&list, list.tail);
    LONGS_EQUAL(0, list.length);

    // Node 1
    POINTERS_EQUAL(NULL, node1.next);
    POINTERS_EQUAL(NULL, node1.prev);
    POINTERS_EQUAL(NULL, node1.list);
}

TEST(List, Remove_2_First)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);

    list_remove(list_get_first(&list));

    // List head
    POINTERS_EQUAL(&node2, list.head);
    POINTERS_EQUAL(&node2, list.tail);
    LONGS_EQUAL(1, list.length);

    // Node 1
    POINTERS_EQUAL(NULL, node1.next);
    POINTERS_EQUAL(NULL, node1.prev);
    POINTERS_EQUAL(NULL, node1.list);

    // Node 2
    POINTERS_EQUAL(&list, node2.next);
    POINTERS_EQUAL(&list, node2.prev);
    POINTERS_EQUAL(&list, node2.list);
}

TEST(List, Remove_2_Last)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);

    list_remove(list_get_last(&list));

    // List head
    POINTERS_EQUAL(&node1, list.head);
    POINTERS_EQUAL(&node1, list.tail);
    LONGS_EQUAL(1, list.length);

    // Node 1
    POINTERS_EQUAL(&list, node1.next);
    POINTERS_EQUAL(&list, node1.prev);
    POINTERS_EQUAL(&list, node1.list);

    // Node 2
    POINTERS_EQUAL(NULL, node2.next);
    POINTERS_EQUAL(NULL, node2.prev);
    POINTERS_EQUAL(NULL, node2.list);
}

TEST(List, Remove_3_First)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);
    list_insert_last(&list, &node3);

    list_remove(list_get_first(&list));

    // List head
    POINTERS_EQUAL(&node2, list.head);
    POINTERS_EQUAL(&node3, list.tail);
    LONGS_EQUAL(2, list.length);

    // Node 1
    POINTERS_EQUAL(NULL, node1.next);
    POINTERS_EQUAL(NULL, node1.prev);
    POINTERS_EQUAL(NULL, node1.list);

    // Node 2
    POINTERS_EQUAL(&node3, node2.next);
    POINTERS_EQUAL(&list, node2.prev);
    POINTERS_EQUAL(&list, node2.list);

    // Node 3
    POINTERS_EQUAL(&list, node3.next);
    POINTERS_EQUAL(&node2, node3.prev);
    POINTERS_EQUAL(&list, node3.list);
}

TEST(List, Remove_3_Last)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);
    list_insert_last(&list, &node3);

    list_remove(list_get_last(&list));

    // List head
    POINTERS_EQUAL(&node1, list.head);
    POINTERS_EQUAL(&node2, list.tail);
    LONGS_EQUAL(2, list.length);

    // Node 1
    POINTERS_EQUAL(&node2, node1.next);
    POINTERS_EQUAL(&list, node1.prev);
    POINTERS_EQUAL(&list, node1.list);

    // Node 2
    POINTERS_EQUAL(&list, node2.next);
    POINTERS_EQUAL(&node1, node2.prev);
    POINTERS_EQUAL(&list, node2.list);

    // Node 3
    POINTERS_EQUAL(NULL, node3.next);
    POINTERS_EQUAL(NULL, node3.prev);
    POINTERS_EQUAL(NULL, node3.list);
}

TEST(List, Remove_3_Middle)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);
    list_insert_last(&list, &node3);

    list_remove(list_get_first(&list)->next);

    // List head
    POINTERS_EQUAL(&node1, list.head);
    POINTERS_EQUAL(&node3, list.tail);
    LONGS_EQUAL(2, list.length);

    // Node 1
    POINTERS_EQUAL(&node3, node1.next);
    POINTERS_EQUAL(&list, node1.prev);
    POINTERS_EQUAL(&list, node1.list);

    // Node 2
    POINTERS_EQUAL(NULL, node2.next);
    POINTERS_EQUAL(NULL, node2.prev);
    POINTERS_EQUAL(NULL, node2.list);

    // Node 3
    POINTERS_EQUAL(&list, node3.next);
    POINTERS_EQUAL(&node1, node3.prev);
    POINTERS_EQUAL(&list, node3.list);
}

TEST(List, Empty_True)
{
    LONGS_EQUAL(1, list_empty(&list));
}

TEST(List, Empty_True_2)
{
    list_insert_last(&list, &node1);
    list_remove(&node1);
    LONGS_EQUAL(1, list_empty(&list));
}

TEST(List, Empty_False)
{
    list_insert_last(&list, &node1);
    LONGS_EQUAL(0, list_empty(&list));
}

TEST(List, Empty_False_2)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);
    LONGS_EQUAL(0, list_empty(&list));
}

TEST(List, MoveFirstToLast_NoNode)
{
    list_move_first_to_last(&list);

    // List head
    POINTERS_EQUAL(&list, list.head);
    POINTERS_EQUAL(&list, list.tail);
    LONGS_EQUAL(0, list.length);
}

TEST(List, MoveFirstToLast_OneNode)
{
    list_insert_last(&list, &node1);

    list_move_first_to_last(&list);

    // List head
    POINTERS_EQUAL(&node1, list.head);
    POINTERS_EQUAL(&node1, list.tail);
    LONGS_EQUAL(1, list.length);

    // Node 1
    POINTERS_EQUAL(&list, node1.next);
    POINTERS_EQUAL(&list, node1.prev);
    POINTERS_EQUAL(&list, node1.list);
}

TEST(List, MoveFirstToLast_TwoNodes)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);

    list_move_first_to_last(&list);

    // List head
    POINTERS_EQUAL(&node2, list.head);
    POINTERS_EQUAL(&node1, list.tail);
    LONGS_EQUAL(2, list.length);

    // Node 1
    POINTERS_EQUAL(&list, node1.next);
    POINTERS_EQUAL(&node2, node1.prev);
    POINTERS_EQUAL(&list, node1.list);

    // Node 2
    POINTERS_EQUAL(&node1, node2.next);
    POINTERS_EQUAL(&list, node2.prev);
    POINTERS_EQUAL(&list, node2.list);
}

TEST(List, MoveFirstToLast_ThreeNodes)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);
    list_insert_last(&list, &node3);

    list_move_first_to_last(&list);

    // List head
    POINTERS_EQUAL(&node2, list.head);
    POINTERS_EQUAL(&node1, list.tail);
    LONGS_EQUAL(3, list.length);

    // Node 1
    POINTERS_EQUAL(&list, node1.next);
    POINTERS_EQUAL(&node3, node1.prev);
    POINTERS_EQUAL(&list, node1.list);

    // Node 2
    POINTERS_EQUAL(&node3, node2.next);
    POINTERS_EQUAL(&list, node2.prev);
    POINTERS_EQUAL(&list, node2.list);

    // Node 3
    POINTERS_EQUAL(&node1, node3.next);
    POINTERS_EQUAL(&node2, node3.prev);
    POINTERS_EQUAL(&list, node3.list);
}
