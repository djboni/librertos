/* Copyright (c) 2022 Djones A. Boni - MIT License */

#ifndef LIBRERTOS_PORT_H_
#define LIBRERTOS_PORT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "librertos_proj.h"

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

    #if __cplusplus >= 201103L
        #define NOEXCEPT_FALSE noexcept(false)
    #else
        #define NOEXCEPT_FALSE
    #endif

/*
 * InterruptsBalanced will throw an exception on destruction if there is an
 * imbalance on the increments/decrements.
 *
 * An increment must must be followed by a decrement.
 * Must decrement first.
 * Cannot increment or decrement multiple times.
 *
 * Example:
 *
 * InterruptsBalanced balanced;
 * ++balanced;
 * --balanced;
 */
class InterruptsBalanced
{
public:
    struct Imbalanced
    {
    };

    InterruptsBalanced(bool strict_order = true)
        : balance(0), already_thrown(false), strict_order(strict_order)
    {
    }

    ~InterruptsBalanced() NOEXCEPT_FALSE { throw_if_unbalanced(); }

    void operator++()
    {
        if (strict_order)
            throw_if_unbalanced();
        ++balance;
    }

    void operator--()
    {
        --balance;
        if (strict_order)
            throw_if_unbalanced();
    }

private:
    void throw_if_unbalanced()
    {
        if (balance != 0 && !already_thrown)
        {
            already_thrown = true;
            throw Imbalanced();
        }
    }

    int balance;
    bool already_thrown;
    bool strict_order;
};

    /*
     * On C++ we have destructors and exceptions, allowing us to automatically
     * test the balance of the critical sections.
     *
     * An exception is thrown if there is an imbalance.
     */

    #define INTERRUPTS_VAL() InterruptsBalanced __balanced_interrupts(false);
    #define INTERRUPTS_DISABLE() ++__balanced_interrupts
    #define INTERRUPTS_ENABLE() --__balanced_interrupts

    #define CRITICAL_VAL() InterruptsBalanced __balanced_critical(true);
    #define CRITICAL_ENTER() ++__balanced_critical
    #define CRITICAL_EXIT() --__balanced_critical

#else /* __cplusplus */

    /*
     * On C we do not have destructors or exceptions, so we are unable to
     * automatically test the balance of the critical sections.
     *
     * These macros may help when debugging: the variable __balanced_interrupts
     * should always be either 0 or 1 and should be 0 at the end of the
     * function.
     */

    #define INTERRUPTS_VAL() int __balanced_interrupts = 0
    #define INTERRUPTS_DISABLE() ++__balanced_interrupts
    #define INTERRUPTS_ENABLE() --__balanced_interrupts

    #define CRITICAL_VAL() int __balanced_critical = 0
    #define CRITICAL_ENTER() ++__balanced_critical
    #define CRITICAL_EXIT() --__balanced_critical

#endif /* __cplusplus */

#endif /* LIBRERTOS_PORT_H_ */
