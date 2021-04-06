#!/usr/bin/python3
import unittest
from unit_test_c_with_python import load, FFIMocks

################################################################################

module_name = 'mutex_'

source_files = [
  '../source/mutex.c',
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

class MutexInit(unittest.TestCase):

    def setUp(self):
        # Mutex
        self.pmtx = ffi.new("struct mutex_t[1]")
        self.mtx = self.pmtx[0]
        # Tasks
        self.ptask0 = ffi.cast("struct task_t*", 0)
        self.ptask1 = ffi.cast("struct task_t*", 1)
        # Mocks
        Mocks.ResetMocks()

    def testInit(self):
        # Initialize mutex
        module.MutexInit(self.pmtx)
        # Values
        self.assertEqual(self.mtx.count, 0)
        self.assertEqual(self.mtx.mutex_owner_ptr, self.ptask0)
        # Calls
        Mocks.OSEventRInit.assert_called_once_with(self.mtx.event)

class MutexLock(unittest.TestCase):

    def setUp(self):
        # Mutex
        self.pmtx = ffi.new("struct mutex_t[1]")
        self.mtx = self.pmtx[0]
        module.MutexInit(self.pmtx)
        # Tasks
        self.ptask0 = ffi.cast("struct task_t*", 0)
        self.ptask1 = ffi.cast("struct task_t*", 1)
        self.ptask2 = ffi.cast("struct task_t*", 2)
        # Mocks
        Mocks.ResetMocks()

    def testLockAnUnlockedMutex(self):
        # Simulate Task1 locking
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret = module.MutexLock(self.pmtx)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(ret, True)
        self.assertEqual(self.mtx.count, 1)
        self.assertEqual(self.mtx.mutex_owner_ptr, self.ptask1)
        # Calls
        Mocks.GetCurrentTask.assert_called_once_with()

    def testLockAnOwnedMutex(self):
        # Simulate Task1 locking twice
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret1 = module.MutexLock(self.pmtx)
        ret2 = module.MutexLock(self.pmtx)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(ret1, True)
        self.assertEqual(ret2, True)
        self.assertEqual(self.mtx.count, 2)
        self.assertEqual(self.mtx.mutex_owner_ptr, self.ptask1)
        # Calls
        called_mock = Mocks.GetCurrentTask
        expected_calls = [
            [],
            [],
        ]
        called_mock.assert_has_calls(expected_calls)
        self.assertEqual(called_mock.call_count, len(expected_calls))

    def testLockALockedMutex(self):
        # Simulate Task1 locking
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret1 = module.MutexLock(self.pmtx)
        # Simulate Task2 locking
        Mocks.GetCurrentTask.return_value = self.ptask2
        ret2 = module.MutexLock(self.pmtx)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(ret1, True)
        self.assertEqual(ret2, False)
        self.assertEqual(self.mtx.count, 1)
        self.assertEqual(self.mtx.mutex_owner_ptr, self.ptask1)
        # Calls
        called_mock = Mocks.GetCurrentTask
        expected_calls = [
            [],
            [],
        ]
        called_mock.assert_has_calls(expected_calls)
        self.assertEqual(called_mock.call_count, len(expected_calls))

class MutexUnlock(unittest.TestCase):

    def setUp(self):
        # Mutex
        self.pmtx = ffi.new("struct mutex_t[1]")
        self.mtx = self.pmtx[0]
        module.MutexInit(self.pmtx)
        # Tasks
        self.ptask0 = ffi.cast("struct task_t*", 0)
        self.ptask1 = ffi.cast("struct task_t*", 1)
        self.ptask2 = ffi.cast("struct task_t*", 2)
        # Mocks
        Mocks.ResetMocks()

    def testUnLockAnUnlockedMutex(self):
        # Simulate Task1 unlocking
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret = module.MutexUnlock(self.pmtx)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(ret, False)
        self.assertEqual(self.mtx.count, 0)
        self.assertEqual(self.mtx.mutex_owner_ptr, self.ptask0)
        # Calls
        Mocks.OSEventUnblockTasks.assert_not_called()

    def testUnlockAnOwnedMutex(self):
        # Simulate Task1 locking and unlocking
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret1 = module.MutexLock(self.pmtx)
        ret2 = module.MutexUnlock(self.pmtx)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(ret1, True)
        self.assertEqual(ret2, True)
        self.assertEqual(self.mtx.count, 0)
        self.assertEqual(self.mtx.mutex_owner_ptr, self.ptask0)
        # Calls
        Mocks.GetCurrentTask.assert_called_once_with()

    def testUnlockAnOwnedMutexMultiple(self):
        # Simulate Task1 locking twich and unlocking twice
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret1 = module.MutexLock(self.pmtx)
        ret2 = module.MutexLock(self.pmtx)
        ret3 = module.MutexUnlock(self.pmtx)
        ret4 = module.MutexUnlock(self.pmtx)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(ret1, True)
        self.assertEqual(ret2, True)
        self.assertEqual(ret1, True)
        self.assertEqual(ret4, True)
        self.assertEqual(self.mtx.count, 0)
        self.assertEqual(self.mtx.mutex_owner_ptr, self.ptask0)
        # Calls
        called_mock = Mocks.GetCurrentTask
        expected_calls = [
            [],
            [],
        ]
        called_mock.assert_has_calls(expected_calls)
        self.assertEqual(called_mock.call_count, len(expected_calls))

    def testLockAMutexLockedByOtherTask(self):
        # Implementation allows it an interrupt or task unlock a mutex locked
        # by another task.
        # Simulate Task1 locking
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret1 = module.MutexLock(self.pmtx)
        # Simulate Task2 unlocking
        Mocks.GetCurrentTask.return_value = self.ptask2
        ret2 = module.MutexUnlock(self.pmtx)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(ret1, True)
        self.assertEqual(ret2, True)
        self.assertEqual(self.mtx.count, 0)
        self.assertEqual(self.mtx.mutex_owner_ptr, self.ptask0)
        # Calls
        Mocks.GetCurrentTask.assert_called_once_with()

    def testUnlockUnblocksPendingTask(self):
        # Simulate Task1 locking
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret1 = module.MutexLock(self.pmtx)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Simulate one pended task
        self.mtx.event.list_read.length = 1
        ret2 = module.MutexUnlock(self.pmtx)
        # Values
        self.assertEqual(ret1, True)
        self.assertEqual(ret2, True)
        # Calls
        Mocks.OSEventUnblockTasks.assert_called_once_with(self.mtx.event.list_read)

class MutexGetCount(unittest.TestCase):

    def setUp(self):
        # Mutex
        self.pmtx = ffi.new("struct mutex_t[1]")
        self.mtx = self.pmtx[0]
        module.MutexInit(self.pmtx)
        # Mocks
        Mocks.ResetMocks()

    def testGetCount(self):
        self.assertEqual(module.MutexGetCount(self.pmtx), 0)
        self.assertEqual(module.MutexLock(self.pmtx), True)
        self.assertEqual(module.MutexGetCount(self.pmtx), 1)
        self.assertEqual(module.MutexLock(self.pmtx), True)
        self.assertEqual(module.MutexGetCount(self.pmtx), 2)
        self.assertEqual(module.MutexUnlock(self.pmtx), True)
        self.assertEqual(module.MutexGetCount(self.pmtx), 1)
        self.assertEqual(module.MutexUnlock(self.pmtx), True)
        self.assertEqual(module.MutexGetCount(self.pmtx), 0)

class MutexGetMutexOwner(unittest.TestCase):

    def setUp(self):
        # Mutex
        self.pmtx = ffi.new("struct mutex_t[1]")
        self.mtx = self.pmtx[0]
        module.MutexInit(self.pmtx)
        # Tasks
        self.ptask0 = ffi.cast("struct task_t*", 0)
        self.ptask1 = ffi.cast("struct task_t*", 1)
        self.ptask2 = ffi.cast("struct task_t*", 2)
        # Mocks
        Mocks.ResetMocks()

    def testGetOwner(self):
        self.assertEqual(module.MutexGetOwner(self.pmtx), self.ptask0)
        # Simulate Task1 locking
        Mocks.GetCurrentTask.return_value = self.ptask1
        self.assertEqual(module.MutexLock(self.pmtx), True)
        self.assertEqual(module.MutexGetOwner(self.pmtx), self.ptask1)
        self.assertEqual(module.MutexUnlock(self.pmtx), True)
        self.assertEqual(module.MutexGetOwner(self.pmtx), self.ptask0)
        # Simulate Task2 locking
        Mocks.GetCurrentTask.return_value = self.ptask2
        self.assertEqual(module.MutexLock(self.pmtx), True)
        self.assertEqual(module.MutexGetOwner(self.pmtx), self.ptask2)
        self.assertEqual(module.MutexUnlock(self.pmtx), True)
        self.assertEqual(module.MutexGetOwner(self.pmtx), self.ptask0)

class MutexTakePend(unittest.TestCase):

    def setUp(self):
        # Mutex
        self.pmtx = ffi.new("struct mutex_t[1]")
        self.mtx = self.pmtx[0]
        # Tasks
        self.ptask0 = ffi.cast("struct task_t*", 0)
        self.ptask1 = ffi.cast("struct task_t*", 1)
        self.ptask2 = ffi.cast("struct task_t*", 2)
        # Mocks
        Mocks.ResetMocks()

    def testLockIfUnlocked(self):
        wait = 0
        # Simulate Task1 locking
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret = module.MutexLockPend(self.pmtx, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(ret, True)
        self.assertEqual(self.mtx.count, 1)
        # Calls
        Mocks.GetCurrentTask.assert_called_once_with()
        Mocks.OSEventPrePendTask.assert_not_called()
        Mocks.OSEventPendTask.assert_not_called()

    def testReturnIfLockedAndZeroTimeout(self):
        wait = 0
        # Simulate Task1 locking
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret1 = module.MutexLockPend(self.pmtx, wait)
        Mocks.GetCurrentTask.return_value = self.ptask2
        ret2 = module.MutexLockPend(self.pmtx, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(ret1, True)
        self.assertEqual(ret2, False)
        self.assertEqual(self.mtx.count, 1)
        # Calls
        called_mock = Mocks.GetCurrentTask
        expected_calls = [
            [],
            [],
        ]
        called_mock.assert_has_calls(expected_calls)
        self.assertEqual(called_mock.call_count, len(expected_calls))
        Mocks.OSEventPrePendTask.assert_not_called()
        Mocks.OSEventPendTask.assert_not_called()

    def testPendIfLockedAndTimeout(self):
        wait = 1
        # Simulate Task1 locking
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret1 = module.MutexLockPend(self.pmtx, wait)
        # Simulate Task2 locking
        Mocks.GetCurrentTask.return_value = self.ptask2
        ret2 = module.MutexLockPend(self.pmtx, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(ret1, True)
        self.assertEqual(ret2, False)
        self.assertEqual(self.mtx.count, 1)
        # Calls
        called_mock = Mocks.GetCurrentTask
        expected_calls = [
            [],
            [],
            [],
        ]
        called_mock.assert_has_calls(expected_calls)
        self.assertEqual(called_mock.call_count, len(expected_calls))
        Mocks.OSEventPrePendTask.assert_called_once_with(self.mtx.event.list_read, self.ptask2)
        Mocks.OSEventPendTask.assert_called_once_with(self.mtx.event.list_read, self.ptask2, wait)

class MutexPend(unittest.TestCase):

    def setUp(self):
        # Mutex
        self.pmtx = ffi.new("struct mutex_t[1]")
        self.mtx = self.pmtx[0]
        # Tasks
        self.ptask0 = ffi.cast("struct task_t*", 0)
        self.ptask1 = ffi.cast("struct task_t*", 1)
        self.ptask2 = ffi.cast("struct task_t*", 2)
        # Mocks
        Mocks.ResetMocks()

    def testReturnIfZeroTimeout(self):
        wait = 0
        # Simulate Task1 locking
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret = module.MutexLock(self.pmtx)
        # Simulate Task2 pending
        Mocks.GetCurrentTask.return_value = self.ptask2
        module.MutexPend(self.pmtx, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(ret, True)
        self.assertEqual(self.mtx.count, 1)
        # Calls
        Mocks.GetCurrentTask.assert_called_once_with()
        Mocks.OSEventPrePendTask.assert_not_called()
        Mocks.OSEventPendTask.assert_not_called()

    def testReturnIfUnlocked(self):
        wait = 1
        # Simulate Task1 pending
        Mocks.GetCurrentTask.return_value = self.ptask1
        module.MutexPend(self.pmtx, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(self.mtx.count, 0)
        # Calls
        Mocks.GetCurrentTask.assert_called_once_with()
        Mocks.OSEventPrePendTask.assert_not_called()
        Mocks.OSEventPendTask.assert_not_called()

    def testReturnIfOwned(self):
        wait = 1
        # Simulate Task1 locking and pending
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret = module.MutexLock(self.pmtx)
        module.MutexPend(self.pmtx, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(ret, True)
        self.assertEqual(self.mtx.count, 1)
        # Calls
        called_mock = Mocks.GetCurrentTask
        expected_calls = [
            [],
            [],
        ]
        called_mock.assert_has_calls(expected_calls)
        self.assertEqual(called_mock.call_count, len(expected_calls))
        Mocks.OSEventPrePendTask.assert_not_called()
        Mocks.OSEventPendTask.assert_not_called()

    def testPendIfTaken(self):
        wait = 1
        # Simulate Task1 locking
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret = module.MutexLock(self.pmtx)
        # Simulate Task2 pending
        Mocks.GetCurrentTask.return_value = self.ptask2
        module.MutexPend(self.pmtx, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(self.mtx.count, 1)
        # Calls
        called_mock = Mocks.GetCurrentTask
        expected_calls = [
            [],
            [],
        ]
        called_mock.assert_has_calls(expected_calls)
        self.assertEqual(called_mock.call_count, len(expected_calls))
        Mocks.OSEventPrePendTask.assert_called_once_with(self.mtx.event.list_read, self.ptask2)
        Mocks.OSEventPendTask.assert_called_once_with(self.mtx.event.list_read, self.ptask2, wait)

################################################################################

if __name__ == "__main__":
    unittest.main()
