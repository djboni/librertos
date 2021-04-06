#!/usr/bin/python3
import unittest
import os, random
from unit_test_c_with_python import load, FFIMocks

################################################################################

LIBRERTOS_MAX_PRIORITY = 3
LIBRERTOS_PREEMPTION = 1
LIBRERTOS_PREEMPT_LIMIT = 0
LIBRERTOS_SOFTWARETIMERS = 1

# Not implemented
LIBRERTOS_STATISTICS = 0
LIBRERTOS_STATE_GUARDS = 0

module_name = 'librertos_'

source_files = [
    '../source/librertos.c',
    '../source/librertos_list.c',
    '../source/librertos_event.c',
    '../source/librertos_state.c',
]

include_paths = [
    '.',
    '../include',
]

compiler_options = [
    '-std=c90',
    '-pedantic',
    '-DLIBRERTOS_MAX_PRIORITY=%d' % LIBRERTOS_MAX_PRIORITY,
    '-DLIBRERTOS_PREEMPTION=%d' % LIBRERTOS_PREEMPTION,
    '-DLIBRERTOS_PREEMPT_LIMIT=%d' % LIBRERTOS_PREEMPT_LIMIT,
    '-DLIBRERTOS_SOFTWARETIMERS=%d' % LIBRERTOS_SOFTWARETIMERS,
    '-DLIBRERTOS_STATISTICS=%d' % LIBRERTOS_STATISTICS,
    '-DLIBRERTOS_STATE_GUARDS=%d' % LIBRERTOS_STATE_GUARDS,
]

module, ffi = load(source_files, include_paths, compiler_options, module_name=module_name, en_code_coverage=True)

############################################################################
# Imports

#import test_list
#test_list.module = module
#test_list.ffi = ffi
#test_list.Mocks = Mocks
#from test_list import *

#import test_event
#test_event.module = module
#test_event.ffi = ffi
#test_event.Mocks = Mocks
#from test_event import *

############################################################################
# Undefinded functions

Mocks = FFIMocks()

# projdefs.h
Mocks.CreateMock(ffi, 'InterruptsEnable', return_value=None)
Mocks.CreateMock(ffi, 'InterruptsDisable', return_value=None)
Mocks.CreateMock(ffi, 'CriticalVal', return_value=None)
Mocks.CreateMock(ffi, 'CriticalEnter', return_value=None)
Mocks.CreateMock(ffi, 'CriticalExit', return_value=None)
Mocks.CreateMock(ffi, 'LibrertosTestConcurrentAccess', return_value=None)

# librertos.h
if LIBRERTOS_STATISTICS:
    Mocks.CreateMock(ffi, 'LibrertosSystemRunTime', return_value=0)

############################################################################

class LibrertosInit(unittest.TestCase):

    def testLibrertosInit(self):
        # Initialization
        module.LibrertosInit()
        ptask0 = ffi.cast("struct task_t*", 0)

        # Values

        # Scheduler locked
        self.assertEqual(module.OS_State.scehduler_lock, 1)

        # Scheduler nas nothing to do
        self.assertEqual(module.OS_State.scheduler_unlock_todo, 0)

        # No task running
        self.assertEqual(module.OS_State.current_tcb_ptr, ptask0)

        if LIBRERTOS_PREEMPTION:
            # No higher priority task ready
            self.assertEqual(module.OS_State.higher_ready_task, 0)

        for i in range(LIBRERTOS_MAX_PRIORITY):
            self.assertEqual(module.OS_State.task_ptr[i], ptask0)

        # Tick and delayed ticks zeroed
        self.assertEqual(module.OS_State.tick, 0)
        self.assertEqual(module.OS_State.delayed_ticks, 0)

        # Blocked task list pointers overflowed and not overflowed initialized
        self.assertEqual(module.OS_State.blocked_task_list_not_overflowed_ptr, ffi.addressof(module.OS_State.blocked_task_list_1))
        self.assertEqual(module.OS_State.blocked_task_list_overflowed_ptr, ffi.addressof(module.OS_State.blocked_task_list_2))

        if LIBRERTOS_SOFTWARETIMERS:
            # Timer task last run zeroed
            self.assertEqual(module.OS_State.task_timer_last_run, 0)

            # Timer index pointer null
            self.assertEqual(module.OS_State.timer_index_ptr, ffi.addressof(module.OS_State.timer_list))

        # TODO
        if 0:
            called_mock = Mocks.OSListHeadInit
            expected_calls = [
                # Pending ready tasks list initialized
                Mocks.call(ffi.addressof(module.OS_State.pending_ready_task_list)),
                # Blocked tasks list 1 initialized
                Mocks.call(ffi.addressof(module.OS_State.blocked_task_list_1)),
                # Blocked tasks list 2 initialized
                Mocks.call(ffi.addressof(module.OS_State.blocked_task_list_2)),
            ]
            if LIBRERTOS_SOFTWARETIMERS:
                expected_calls += [
                    # Blocked tasks list 2 initialized
                    Mocks.call(ffi.addressof(module.OS_State.timer_list)),
                    # Blocked tasks list 2 initialized
                    Mocks.call(ffi.addressof(module.OS_State.timer_unordered_list)),
                ]
            called_mock.assert_has_calls(expected_calls)
            self.assertEqual(called_mock.call_count, len(expected_calls))

        if LIBRERTOS_STATISTICS:
            raise Exception(NotImplemented)

        if LIBRERTOS_STATE_GUARDS:
            raise Exception(NotImplemented)

class SchedulerLock(unittest.TestCase):

    def testCall(self):
        module.LibrertosInit()
        scehduler_lock_value = module.OS_State.scehduler_lock
        module.SchedulerLock()
        # Values
        self.assertEqual(module.OS_State.scehduler_lock, scehduler_lock_value + 1)

class OSTickInvertBlockedTasksLists(unittest.TestCase):

    def testCallOnce(self):
        ptr1 = module.OS_State.blocked_task_list_not_overflowed_ptr
        ptr2 = module.OS_State.blocked_task_list_overflowed_ptr
        module.OSTickInvertBlockedTasksLists()
        self.assertEqual(module.OS_State.blocked_task_list_not_overflowed_ptr, ptr2)
        self.assertEqual(module.OS_State.blocked_task_list_overflowed_ptr, ptr1)

    def testCallTwice(self):
        ptr1 = module.OS_State.blocked_task_list_not_overflowed_ptr
        ptr2 = module.OS_State.blocked_task_list_overflowed_ptr
        module.OSTickInvertBlockedTasksLists()
        module.OSTickInvertBlockedTasksLists()
        self.assertEqual(module.OS_State.blocked_task_list_not_overflowed_ptr, ptr1)
        self.assertEqual(module.OS_State.blocked_task_list_overflowed_ptr, ptr2)

class OSTickUnblockTimedoutTasks(unittest.TestCase):

    def testCall(self):
        #Real.OSListInsertAfter()
        #module.OSListRemove(ffi.new("struct task_list_node_t *"))
        pass

class OSTickUnblockPendingReadyTasks(unittest.TestCase):

    def testCall(self):
        pass

class OSScheduleTask(unittest.TestCase):

    def testCall(self):
        pass

class OSSchedulerReal(unittest.TestCase):

    def testCall(self):
        pass

class SchedulerUnlock(unittest.TestCase):

    def testSchedulerLockedTwice(self):
        module.LibrertosInit()
        module.SchedulerLock()
        scehduler_lock_value = module.OS_State.scehduler_lock
        module.SchedulerUnlock()
        self.assertEqual(module.OS_State.scehduler_lock, scehduler_lock_value - 1)
if 0:
    def testSchedulerLockedOnce(self):
        module.LibrertosInit()
        scehduler_lock_value = module.OS_State.scehduler_lock
        module.SchedulerUnlock()
        self.assertEqual(module.OS_State.scehduler_lock, scehduler_lock_value - 1)

class LibrertosStart(unittest.TestCase):

    def testCall(self):
        module.LibrertosInit()
        module.LibrertosStart()
        # Values
        self.assertEqual(module.OS_State.scehduler_lock, 0)
        # Calls
        Mocks.InterruptsEnable.assert_called_once_with()

class LibrertosTick(unittest.TestCase):

    def setUp(self):
        pass

    def testCall(self):
        pass

class LibrertosScheduler(unittest.TestCase):

    def setUp(self):
        pass

    def testCall(self):
        pass

class LibrertosTaskCreate(unittest.TestCase):

    def setUp(self):
        pass

    def testCall(self):
        pass

class TaskDelay(unittest.TestCase):

    def setUp(self):
        pass

    def testCall(self):
        pass

class TaskResume(unittest.TestCase):

    def setUp(self):
        pass

    def testCall(self):
        pass

class GetTickCount(unittest.TestCase):

    def setUp(self):
        pass

    def testCall(self):
        pass


class LibrertosStateCheck(unittest.TestCase):

    def setUp(self):
        if LIBRERTOS_STATE_GUARDS:
            pass

    def testCall(self):
        if LIBRERTOS_STATE_GUARDS:
            pass

class LibrertosTotalRunTime(unittest.TestCase):

    def setUp(self):
        if LIBRERTOS_STATE_GUARDS:
            pass

    def testCall(self):
        if LIBRERTOS_STATE_GUARDS:
            pass

    class LibrertosNoTaskRunTime(unittest.TestCase):

        def setUp(self):
            pass

        def testCall(self):
            pass

    class LibrertosTaskRunTime(unittest.TestCase):

        def setUp(self):
            pass

        def testCall(self):
            pass

    class LibrertosTaskNumSchedules(unittest.TestCase):

        def setUp(self):
            pass

        def testCall(self):
            pass

############################################################################

if __name__ == '__main__':
    unittest.main()
