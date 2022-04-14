/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "librertos.h"
#include "librertos_impl.h"
#include "tests/utils/librertos_custom_tests.h"

/*
 * Main file: src/librertos.c
 * Also compile: tests/mocks/librertos_assert.cpp
 * Also compile: tests/utils/librertos_test_utils.cpp
 * Also compile: tests/utils/librertos_custom_tests.cpp
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
    list_tester(&list, std::vector<node_t *>{});
}

TEST(List, InsertFirst_1_First)
{
    list_insert_first(&list, &node1);

    list_tester(&list, std::vector<node_t *>{&node1});
}

TEST(List, InsertLast_1_Last)
{
    list_insert_last(&list, &node1);

    list_tester(&list, std::vector<node_t *>{&node1});
}

TEST(List, InsertFirst_2_First)
{
    list_insert_first(&list, &node1);
    list_insert_first(&list, &node2);

    list_tester(&list, std::vector<node_t *>{&node2, &node1});
}

TEST(List, InsertLast_2_Last)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);

    list_tester(&list, std::vector<node_t *>{&node1, &node2});
}

TEST(List, InsertFirst_3_First)
{
    list_insert_first(&list, &node1);
    list_insert_first(&list, &node2);
    list_insert_first(&list, &node3);

    list_tester(&list, std::vector<node_t *>{&node3, &node2, &node1});
}

TEST(List, InsertLast_3_Last)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);
    list_insert_last(&list, &node3);

    list_tester(&list, std::vector<node_t *>{&node1, &node2, &node3});
}

TEST(List, InsertAfter_3_AfterPos)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);
    list_insert_after(&list, &node1, &node3);

    list_tester(&list, std::vector<node_t *>{&node1, &node3, &node2});
}

TEST(List, InsertBefore_3_BeforePos)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);
    list_insert_before(&list, &node2, &node3);

    list_tester(&list, std::vector<node_t *>{&node1, &node3, &node2});
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

    list_tester(&list, std::vector<node_t *>{});
}

TEST(List, Remove_2_First)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);

    list_remove(list_get_first(&list));

    list_tester(&list, std::vector<node_t *>{&node2});
}

TEST(List, Remove_2_Last)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);

    list_remove(list_get_last(&list));

    list_tester(&list, std::vector<node_t *>{&node1});
}

TEST(List, Remove_3_First)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);
    list_insert_last(&list, &node3);

    list_remove(list_get_first(&list));

    list_tester(&list, std::vector<node_t *>{&node2, &node3});
}

TEST(List, Remove_3_Last)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);
    list_insert_last(&list, &node3);

    list_remove(list_get_last(&list));

    list_tester(&list, std::vector<node_t *>{&node1, &node2});
}

TEST(List, Remove_3_Middle)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);
    list_insert_last(&list, &node3);

    list_remove(list_get_first(&list)->next);

    list_tester(&list, std::vector<node_t *>{&node1, &node3});
}

TEST(List, Empty_True)
{
    LONGS_EQUAL(1, list_is_empty(&list));
}

TEST(List, Empty_True_2)
{
    list_insert_last(&list, &node1);
    list_remove(&node1);
    LONGS_EQUAL(1, list_is_empty(&list));
}

TEST(List, Empty_False)
{
    list_insert_last(&list, &node1);
    LONGS_EQUAL(0, list_is_empty(&list));
}

TEST(List, Empty_False_2)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);
    LONGS_EQUAL(0, list_is_empty(&list));
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

    list_tester(&list, std::vector<node_t *>{&node1});
}

TEST(List, MoveFirstToLast_TwoNodes)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);

    list_move_first_to_last(&list);

    list_tester(&list, std::vector<node_t *>{&node2, &node1});
}

TEST(List, MoveFirstToLast_ThreeNodes)
{
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);
    list_insert_last(&list, &node3);

    list_move_first_to_last(&list);

    list_tester(&list, std::vector<node_t *>{&node2, &node3, &node1});
}
