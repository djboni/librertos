/* Copyright (c) 2022 Djones A. Boni - MIT License */

#ifndef LIBRERTOS_CUSTOM_TESTS_H_
#define LIBRERTOS_CUSTOM_TESTS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "librertos.h"

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

    #include <vector>

void list_tester(list_t *list, std::vector<node_t *> nodes);

#endif /* __cplusplus */

#endif /* LIBRERTOS_CUSTOM_TESTS_H_ */
