#include "librertos_custom_tests.h"

#include <cstring>
#include <sstream>
#include <string>

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#ifdef TESTING_TESTER

struct CheckFailed
{
};

    #undef CHECK_TRUE
    #define CHECK_TRUE(x) \
        do \
        { \
            if (!(x)) \
                throw CheckFailed(); \
        } while (0)

    #undef POINTERS_EQUAL
    #define POINTERS_EQUAL(x, y) \
        do \
        { \
            if ((void *)(x) != (void *)(y)) \
                throw CheckFailed(); \
        } while (0)

#endif /* TESTING_TESTER */

void list_tester(list_t *list, std::vector<node_t *> nodes)
{
    // Size is OK
    CHECK_TRUE(list->length == nodes.size());

    // List head and tail are OK
    if (list->length == 0)
    {
        POINTERS_EQUAL(list, list->head);
        POINTERS_EQUAL(list, list->tail);
        return;
    }
    else
    {
        POINTERS_EQUAL(nodes[0], list->head);
        POINTERS_EQUAL(nodes[nodes.size() - 1], list->tail);
    }

    // Forward list
    for (auto node = nodes.begin(); node != nodes.end() - 1; node++)
        POINTERS_EQUAL(*(node + 1), (*node)->next);
    POINTERS_EQUAL(list, nodes[nodes.size() - 1]->next);

    // Backwards list
    for (auto node = nodes.rbegin(); node != nodes.rend() - 1; node++)
        POINTERS_EQUAL(*(node + 1), (*node)->prev);
    POINTERS_EQUAL(list, nodes[0]->prev);

    // List
    for (auto node = nodes.begin(); node != nodes.end() - 1; node++)
        POINTERS_EQUAL(list, (*node)->list);
    POINTERS_EQUAL(list, nodes[nodes.size() - 1]->list);
}

test_t test;

void test_init()
{
    memset(&test, 0x5a, sizeof(test));

    test.used_tasks = 0;

    librertos_init();
}

std::vector<task_t *> test_create_tasks(
    std::vector<uint8_t> prio,
    task_function_t func,
    std::vector<void *> param = {NULL})
{
    std::vector<task_t *> task_vec;

    if (param.size() == 0)
    {
        for (uint8_t i = 0; i < prio.size(); i++)
            param.push_back(NULL);
    }
    else if (param.size() == 1)
    {
        for (uint8_t i = 1; i < prio.size(); i++)
            param.push_back(param[0]);
    }
    else
    {
        /* Do nothing. */
    }

    LONGS_EQUAL(prio.size(), param.size());

    for (uint8_t i = 0; i < prio.size(); i++)
    {
        task_t *task = &test.task[test.used_tasks++];
        CHECK_TRUE(test.used_tasks <= NUM_TASKS);
        task_vec.push_back(task);

        librertos_create_task(prio[i], task, func, param[i]);
    }

    return task_vec;
}

/* Convert a std::vector<int> to a string. */
SimpleString StringFrom(std::vector<int> &vec)
{
    std::stringstream s;

    s << "[";

    for (unsigned i = 0; i < vec.size(); i++)
    {
        if (i != 0)
            s << ", ";
        s << vec[i];
    }

    s << "]";

    return SimpleString{s.str().c_str()};
}
