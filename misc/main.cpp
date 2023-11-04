/* Copyright (c) 2022 Djones A. Boni - MIT License */

#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTest/TestRegistry.h"
#include "CppUTestExt/MockSupportPlugin.h"

#include <cstdio>

struct MockSupportPluginComparator : MockNamedValueComparator {
    MockSupportPluginComparator() {
    }
    ~MockSupportPluginComparator() {
    }

    bool isEqual(const void *object1, const void *object2) {
        return object1 == object2;
    }

    SimpleString valueToString(const void *object) {
        char str[32];

        std::snprintf(&str[0], sizeof(str), "%p", object);
        str[sizeof(str) - 1] = '\0';

        return SimpleString(str);
    }
};

int main(int argc, char **argv) {
    MockSupportPluginComparator comparator;
    MockSupportPlugin mock_plugin;

    mock_plugin.installComparator("MyDummyType", comparator);
    TestRegistry::getCurrentRegistry()->installPlugin(&mock_plugin);

    return CommandLineTestRunner::RunAllTests(argc, argv);
}
