#!/usr/bin/python3
import unittest
from unit_test_c_with_python import load, FFIMocks

################################################################################

module_name = 'semaphore_'

source_files = [
  '../source/semaphore.c',
  '../source/librertos_state.c',
]

include_paths = [
  '.',
  '../include',
]

compiler_options = [
  '-std=c90',
  '-pedantic',
  '-DLIBRERTOS_PREEMPTION=0',
  '-DLIBRERTOS_PREEMPT_LIMIT=0'
]

module, ffi = load(source_files, include_paths, compiler_options, module_name=module_name, en_code_coverage=True)

################################################################################
# Undefinded functions

Mocks = FFIMocks()

# projdefs.h
Mocks.CreateMock(ffi, 'InterruptsEnable', return_value=None)
Mocks.CreateMock(ffi, 'InterruptsDisable', return_value=None)
Mocks.CreateMock(ffi, 'CriticalVal', return_value=None)
Mocks.CreateMock(ffi, 'CriticalEnter', return_value=None)
Mocks.CreateMock(ffi, 'CriticalExit', return_value=None)
Mocks.CreateMock(ffi, 'LibrertosTestConcurrentAccess', return_value=None)

# librertos.c
Mocks.CreateMock(ffi, 'SchedulerLock', return_value=None)
Mocks.CreateMock(ffi, 'SchedulerUnlock', return_value=None)
Mocks.CreateMock(ffi, 'GetCurrentTask', return_value=ffi.cast("struct task_t*", 0))

# librertos_event.c
Mocks.CreateMock(ffi, 'OSEventRInit', return_value=None)
Mocks.CreateMock(ffi, 'OSEventRwInit', return_value=None)
Mocks.CreateMock(ffi, 'OSEventPrePendTask', return_value=None)
Mocks.CreateMock(ffi, 'OSEventPendTask', return_value=None)
Mocks.CreateMock(ffi, 'OSEventUnblockTasks', return_value=None)

################################################################################

class SemaphoreInit(unittest.TestCase):

    def setUp(self):
        # Semaphore
        self.psem = ffi.new("struct semaphore_t[1]")
        self.sem = self.psem[0]
        # Mocks
        Mocks.ResetMocks()

    def testInitBinary(self):
        count, max_ = 1, 1
        module.SemaphoreInit(self.psem, count, max_)
        # Values
        self.assertEqual(self.sem.count, count)
        self.assertEqual(self.sem.max, max_)
        # Calls
        Mocks.OSEventRInit.assert_called_once_with(self.sem.event)

    def testInitCounting(self):
        count, max_ = 3, 5
        module.SemaphoreInit(self.psem, count, max_)
        # Values
        self.assertEqual(self.sem.count, count)
        self.assertEqual(self.sem.max, max_)
        # Calls
        Mocks.OSEventRInit.assert_called_once_with(self.sem.event)

class SemaphoreGetCount(unittest.TestCase):

    def setUp(self):
        # Semaphore
        self.psem = ffi.new("struct semaphore_t[1]")
        self.sem = self.psem[0]

    def testGetCountBinary(self):
        count, max_ = 0, 1
        module.SemaphoreInit(self.psem, count, max_)
        self.assertEqual(module.SemaphoreGetCount(self.psem), count)

    def testGetCountCounter(self):
        count, max_ = 1, 2
        module.SemaphoreInit(self.psem, count, max_)
        self.assertEqual(module.SemaphoreGetCount(self.psem), count)

class SemaphoreGetMax(unittest.TestCase):

    def setUp(self):
        # Semaphore
        self.psem = ffi.new("struct semaphore_t[1]")
        self.sem = self.psem[0]

    def testGetMaxBinary(self):
        count, max_ = 0, 1
        module.SemaphoreInit(self.psem, count, max_)
        self.assertEqual(module.SemaphoreGetMax(self.psem), max_)

    def testGetMaxCounter(self):
        count, max_ = 1, 2
        module.SemaphoreInit(self.psem, count, max_)
        self.assertEqual(module.SemaphoreGetMax(self.psem), max_)

class SemaphoreTake(unittest.TestCase):

    def setUp(self):
        # Semaphore
        self.psem = ffi.new("struct semaphore_t[1]")
        self.sem = self.psem[0]

    def testTakeBinaryTaken(self):
        count, max_ = 0, 1
        module.SemaphoreInit(self.psem, count, max_)
        self.assertEqual(module.SemaphoreTake(self.psem), False)
        self.assertEqual(self.sem.count, 0)
        self.assertEqual(self.sem.max, max_)

    def testTakeBinaryGiven(self):
        count, max_ = 1, 1
        module.SemaphoreInit(self.psem, count, max_)
        self.assertEqual(module.SemaphoreTake(self.psem), True)
        self.assertEqual(self.sem.count, 0)
        self.assertEqual(module.SemaphoreTake(self.psem), False)
        self.assertEqual(self.sem.count, 0)
        self.assertEqual(self.sem.max, max_)

    def testMultipleTakesCounter(self):
        count, max_ = 3, 5
        module.SemaphoreInit(self.psem, count, max_)
        for i in range(count,0,-1):
            self.assertEqual(module.SemaphoreTake(self.psem), True)
            self.assertEqual(self.sem.count, i-1)
        self.assertEqual(module.SemaphoreTake(self.psem), False)
        self.assertEqual(self.sem.count, 0)
        self.assertEqual(self.sem.max, max_)

class SemaphoreGive(unittest.TestCase):

    def setUp(self):
        # Semaphore
        self.psem = ffi.new("struct semaphore_t[1]")
        self.sem = self.psem[0]
        # Mocks
        Mocks.ResetMocks()

    def testGiveBinaryGiven(self):
        count, max_ = 1, 1
        module.SemaphoreInit(self.psem, count, max_)
        self.assertEqual(module.SemaphoreGive(self.psem), False)
        self.assertEqual(self.sem.count, 1)
        self.assertEqual(self.sem.max, max_)
        # Calls
        Mocks.OSEventUnblockTasks.assert_not_called()

    def testGiveBinaryTaken(self):
        count, max_ = 0, 1
        module.SemaphoreInit(self.psem, count, max_)
        self.assertEqual(module.SemaphoreGive(self.psem), True)
        self.assertEqual(self.sem.count, 1)
        self.assertEqual(module.SemaphoreGive(self.psem), False)
        self.assertEqual(self.sem.count, 1)
        self.assertEqual(self.sem.max, max_)
        # Calls
        Mocks.OSEventUnblockTasks.assert_not_called()

    def testMultipleGivesCounter(self):
        count, max_ = 2, 5
        module.SemaphoreInit(self.psem, count, max_)
        for i in range(count,max_):
            self.assertEqual(module.SemaphoreGive(self.psem), True)
            self.assertEqual(self.sem.count, i+1)
        self.assertEqual(module.SemaphoreGive(self.psem), False)
        self.assertEqual(self.sem.count, max_)
        self.assertEqual(self.sem.max, max_)
        # Calls
        Mocks.OSEventUnblockTasks.assert_not_called()

    def testGiveUnlocksPendingTask(self):
        count, max_ = 0, 1
        module.SemaphoreInit(self.psem, count, max_)
        # Simulate one pended task
        self.sem.event.list_read.length = 1
        self.assertEqual(module.SemaphoreGive(self.psem), True)
        # Calls
        Mocks.OSEventUnblockTasks.assert_called_once_with(self.sem.event.list_read)

class SemaphoreTakePend(unittest.TestCase):

    def setUp(self):
        # Semaphore
        self.psem = ffi.new("struct semaphore_t[1]")
        self.sem = self.psem[0]
        # Tasks
        self.ptask0 = ffi.cast("struct task_t*", 0)
        self.ptask1 = ffi.cast("struct task_t*", 1)
        # Mocks
        Mocks.ResetMocks()

    def testTakeIfGiven(self):
        count, max_ = 1, 1
        wait = 0
        module.SemaphoreInit(self.psem, count, max_)
        ret = module.SemaphoreTakePend(self.psem, wait)
        # Values
        self.assertEqual(ret, True)
        self.assertEqual(self.sem.count, 0)
        self.assertEqual(self.sem.max, max_)
        # Calls
        Mocks.GetCurrentTask.assert_not_called()
        Mocks.OSEventPrePendTask.assert_not_called()
        Mocks.OSEventPendTask.assert_not_called()

    def testReturnIfTakenAndZeroTimeout(self):
        count, max_ = 0, 1
        wait = 0
        module.SemaphoreInit(self.psem, count, max_)
        ret = module.SemaphoreTakePend(self.psem, wait)
        # Values
        self.assertEqual(ret, False)
        self.assertEqual(self.sem.count, 0)
        self.assertEqual(self.sem.max, max_)
        # Calls
        Mocks.GetCurrentTask.assert_not_called()
        Mocks.OSEventPrePendTask.assert_not_called()
        Mocks.OSEventPendTask.assert_not_called()

    def testPendIfTakenAndTimeout(self):
        count, max_ = 0, 1
        wait = 1
        module.SemaphoreInit(self.psem, count, max_)
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret = module.SemaphoreTakePend(self.psem, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(ret, False)
        self.assertEqual(self.sem.count, 0)
        self.assertEqual(self.sem.max, max_)
        # Calls
        Mocks.GetCurrentTask.assert_called_once_with()
        Mocks.OSEventPrePendTask.assert_called_once_with(self.sem.event.list_read, self.ptask1)
        Mocks.OSEventPendTask.assert_called_once_with(self.sem.event.list_read, self.ptask1, wait)


class SemaphorePend(unittest.TestCase):

    def setUp(self):
        # Semaphore
        self.psem = ffi.new("struct semaphore_t[1]")
        self.sem = self.psem[0]
        # Tasks
        self.ptask0 = ffi.cast("struct task_t*", 0)
        self.ptask1 = ffi.cast("struct task_t*", 1)
        # Mocks
        Mocks.ResetMocks()

    def testReturnIfZeroTimeout(self):
        count, max_ = 0, 1
        wait = 0
        module.SemaphoreInit(self.psem, count, max_)
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        module.SemaphorePend(self.psem, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(self.sem.count, 0)
        self.assertEqual(self.sem.max, max_)
        # Calls
        Mocks.GetCurrentTask.assert_not_called()
        Mocks.OSEventPrePendTask.assert_not_called()
        Mocks.OSEventPendTask.assert_not_called()

    def testPendIfTaken(self):
        count, max_ = 0, 1
        wait = 1
        module.SemaphoreInit(self.psem, count, max_)
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        module.SemaphorePend(self.psem, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(self.sem.count, 0)
        self.assertEqual(self.sem.max, max_)
        # Calls
        Mocks.GetCurrentTask.assert_called_once_with()
        Mocks.OSEventPrePendTask.assert_called_once_with(self.sem.event.list_read, self.ptask1)
        Mocks.OSEventPendTask.assert_called_once_with(self.sem.event.list_read, self.ptask1, wait)

    def testReturnIfGiven(self):
        count, max_ = 1, 1
        wait = 1
        module.SemaphoreInit(self.psem, count, max_)
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        module.SemaphorePend(self.psem, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(self.sem.count, 1)
        self.assertEqual(self.sem.max, max_)
        # Calls
        Mocks.GetCurrentTask.assert_not_called()
        Mocks.OSEventPrePendTask.assert_not_called()
        Mocks.OSEventPendTask.assert_not_called()

################################################################################

if __name__ == "__main__":
    unittest.main()
