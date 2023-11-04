/* Copyright (c) 2016-2023 Djones A. Boni - MIT License */

#include "librertos.h"
#include "librertos_impl.h"

/*
 * Main file: src/librertos.c
 * Also compile: tests/mocks/librertos_assert.cpp
 * Also compile: tests/utils/librertos_test_utils.cpp
 */

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#define TESTING_TESTER 1
#include "tests/utils/librertos_custom_tests.cpp"

TEST_GROUP (ListTester) {
    list_t list;
    node_t node1, node2, node3;
    int owner1, owner2, owner3;

    void setup() {
        list_init(&list);
        node_init(&node1, &owner1);
        node_init(&node2, &owner2);
        node_init(&node3, &owner3);
    }
    void teardown() {
    }
};

TEST(ListTester, DifferentSize) {
    CHECK_THROWS(
        CheckFailed, list_tester(&list, std::vector<node_t *>{&node1}));
}

TEST(ListTester, LengthZero_HeadIsWrong) {
    list.head = NULL;
    CHECK_THROWS(CheckFailed, list_tester(&list, std::vector<node_t *>{}));
}

TEST(ListTester, LengthZero_TailIsWrong) {
    list.tail = NULL;
    CHECK_THROWS(CheckFailed, list_tester(&list, std::vector<node_t *>{}));
}

TEST(ListTester, LengthZero_OK) {
    list_tester(&list, std::vector<node_t *>{});
}

TEST(ListTester, LengthOne_HeadIsWrong) {
    list_insert_last(&list, &node1);
    list.head = NULL;
    CHECK_THROWS(
        CheckFailed, list_tester(&list, std::vector<node_t *>{&node1}));
}

TEST(ListTester, LengthOne_TailIsWrong) {
    list_insert_last(&list, &node1);
    list.tail = NULL;
    CHECK_THROWS(
        CheckFailed, list_tester(&list, std::vector<node_t *>{&node1}));
}

TEST(ListTester, LengthOne_LastNextIsWrong) {
    list_insert_last(&list, &node1);
    node1.next = NULL;
    CHECK_THROWS(
        CheckFailed, list_tester(&list, std::vector<node_t *>{&node1}));
}

TEST(ListTester, LengthOne_LastPrevIsWrong) {
    list_insert_last(&list, &node1);
    node1.prev = NULL;
    CHECK_THROWS(
        CheckFailed, list_tester(&list, std::vector<node_t *>{&node1}));
}

TEST(ListTester, LengthOne_LastListIsWrong) {
    list_insert_last(&list, &node1);
    node1.list = NULL;
    CHECK_THROWS(
        CheckFailed, list_tester(&list, std::vector<node_t *>{&node1}));
}

TEST(ListTester, LengthOne_OK) {
    list_insert_last(&list, &node1);
    list_tester(&list, std::vector<node_t *>{&node1});
}

TEST(ListTester, LengthTwo_FirstNextIsWrong) {
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);
    node1.next = NULL;
    CHECK_THROWS(
        CheckFailed, list_tester(&list, std::vector<node_t *>{&node1, &node2}));
}

TEST(ListTester, LengthTwo_FirstPrevIsWrong) {
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);
    node2.prev = NULL;
    CHECK_THROWS(
        CheckFailed, list_tester(&list, std::vector<node_t *>{&node1, &node2}));
}

TEST(ListTester, LengthTwo_FirstListIsWrong) {
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);
    node1.list = NULL;
    CHECK_THROWS(
        CheckFailed, list_tester(&list, std::vector<node_t *>{&node1, &node2}));
}

TEST(ListTester, LengthTwo_OK) {
    list_insert_last(&list, &node1);
    list_insert_last(&list, &node2);
    list_tester(&list, std::vector<node_t *>{&node1, &node2});
}
