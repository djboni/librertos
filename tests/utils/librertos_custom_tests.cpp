#include "librertos_custom_tests.h"

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
