/* Copyright (c) 2022 Djones A. Boni - MIT License */

#ifndef LIBRERTOS_PORT_H_
#define LIBRERTOS_PORT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define INTERRUPTS_DISABLE()
#define INTERRUPTS_ENABLE()

#define CRITICAL_VAL()
#define CRITICAL_ENTER()
#define CRITICAL_EXIT()

#ifdef __cplusplus
}
#endif

#endif /* LIBRERTOS_PORT_H_ */
